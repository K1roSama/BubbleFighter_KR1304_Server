#include "application/server_application.h"

#include <charconv>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>

#include "application/local_account_command.h"
#include "features/authentication/account_authentication.h"
#include "features/rooms/match_profile_updates.h"
#include "features/shop/shop_catalog_response.h"
#include "network/connection_heartbeat.h"
#include "network/tcp_server.h"
#include "persistence/my_sql_account_store.h"
#include "persistence/my_sql_character_store.h"
#include "persistence/my_sql_inventory_store.h"
#include "persistence/my_sql_match_store.h"
#include "sessions/menu_snapshot.h"
#include "sessions/player_session.h"

namespace bubble_fighter::server::application {
namespace {
void PrintUsage() {
  std::cout
      << "Status: local ticket authentication and character persistence implemented; full game login pending.\n"
      << "Usage: BubbleFighter.Server --listen-port <0..65535> [--listen-address <IPv4>] [--once] [--database-config <path>] [--idle-timeout-seconds <1..600>] [--match-rules <path>]\n"
      << "Default listen address: 127.0.0.1. Use a local IPv4 address for LAN or 0.0.0.0 for all IPv4 interfaces. Port 0 selects an available port.\n";
  std::cout
      << "Local accounts: --create-account <username> --database-config <path>\n"
      << "Local login: --local-login <username> --database-config <path> --ticket-file <new-path>\n"
      << "Passwords are read from UTF-8 standard input with console echo disabled.\n";
}
}  // namespace

int ServerApplication::Run(int argc, char* argv[]) const {
  std::cout << "BubbleFighter.Server\n";
  if (LocalAccountCommand::IsRequested(argc, argv)) {
    return LocalAccountCommand{}.Run(argc, argv);
  }
  if (argc == 1 || (argc == 2 && std::string_view(argv[1]) == "--help")) {
    PrintUsage();
    return 0;
  }

  bool has_port = false;
  bool has_listen_address = false;
  std::string_view listen_address = "127.0.0.1";
  bool once = false;
  unsigned int port = 0;
  unsigned int idle_timeout_seconds = 30;
  bool has_idle_timeout = false;
  std::filesystem::path database_configuration;
  std::filesystem::path match_rules_path;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "--listen-port" && !has_port && index + 1 < argc) {
      const std::string_view value(argv[++index]);
      const auto result =
          std::from_chars(value.data(), value.data() + value.size(), port);
      if (result.ec != std::errc{} ||
          result.ptr != value.data() + value.size() || port > 65535) {
        std::cerr << "Invalid listen port.\n";
        return 2;
      }
      has_port = true;
    } else if (argument == "--listen-address" && !has_listen_address &&
               index + 1 < argc) {
      listen_address = argv[++index];
      try {
        network::TcpServer::ParseListenAddress(listen_address);
      } catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n';
        return 2;
      }
      has_listen_address = true;
    } else if (argument == "--database-config" &&
               database_configuration.empty() && index + 1 < argc) {
      database_configuration = argv[++index];
    } else if (argument == "--idle-timeout-seconds" && !has_idle_timeout &&
               index + 1 < argc) {
      const std::string_view value(argv[++index]);
      const auto result = std::from_chars(
          value.data(), value.data() + value.size(), idle_timeout_seconds);
      if (result.ec != std::errc{} ||
          result.ptr != value.data() + value.size() ||
          idle_timeout_seconds == 0 || idle_timeout_seconds > 600) {
        std::cerr << "Invalid idle timeout.\n";
        return 2;
      }
      has_idle_timeout = true;
    } else if (argument == "--match-rules" && match_rules_path.empty() &&
               index + 1 < argc) {
      match_rules_path = argv[++index];
    } else if (argument == "--once" && !once) {
      once = true;
    } else {
      std::cerr << "Invalid or duplicate argument: " << argument << '\n';
      PrintUsage();
      return 2;
    }
  }
  if (!has_port) {
    std::cerr << "--listen-port is required to start listening.\n";
    return 2;
  }
  try {
    const auto rules =
        match_rules_path.empty()
            ? features::rooms::MatchRewardPolicy{}
            : features::rooms::MatchRewardPolicy::Load(match_rules_path);
    features::rooms::RoomMatchSettler settler;
    if (!database_configuration.empty()) {
      // Fail before listening if the required receipt migration is absent.
      persistence::MySqlMatchStore check_schema(database_configuration);
      settler = [database_configuration](
                    const features::rooms::RoomMatchRequest& request) {
        return persistence::MySqlMatchStore(database_configuration)
            .Settle(request);
      };
    }
    const auto rooms = features::rooms::RoomRegistry::Create(
        512, {}, {}, std::move(settler), rules);
    // Match rewards outlive individual TCP handlers and even the last
    // room member. The worker holds no room-table lock during retries.
    std::jthread settlement_worker;
    if (!database_configuration.empty()) {
      settlement_worker =
          std::jthread([rooms, database_configuration](std::stop_token stop) {
            std::mutex wait_mutex;
            std::condition_variable_any wake;
            std::unique_lock wait_lock(wait_mutex);
            std::array<std::uint8_t, 16> cursor{};
            auto recover_at = std::chrono::steady_clock::time_point{};
            bool recovery_delayed = false;
            while (!stop.stop_requested()) {
              const auto now = std::chrono::steady_clock::now();
              if (now >= recover_at) {
                try {
                  auto pending =
                      persistence::MySqlMatchStore(database_configuration)
                          .LoadPending(cursor);
                  for (auto& request : pending) {
                    rooms->RecoverSettlement(request);
                  }
                  cursor = pending.empty() ? std::array<std::uint8_t, 16>{}
                                           : pending.back().match_id;
                  recovery_delayed = false;
                } catch (const std::exception&) {
                  if (!recovery_delayed) {
                    std::cerr << "Pending match recovery scan delayed.\n";
                  }
                  recovery_delayed = true;
                }
                recover_at = now + std::chrono::seconds{2};
              }
              try {
                rooms->RetrySettlements();
              } catch (const std::exception&) {
                std::cerr << "Cannot poll pending match settlements.\n";
              }
              wake.wait_for(wait_lock, stop, std::chrono::milliseconds{100},
                            [] { return false; });
            }
          });
    }
    const auto plazas = std::make_shared<features::lobby::PlazaRegistry>();
    const auto create_handler = [database_configuration, idle_timeout_seconds,
                                 rooms, rules,
                                 plazas]() -> network::ConnectionHandlers {
      sessions::PlayerSession::TicketConsumer consume;
      if (!database_configuration.empty()) {
        consume = [database_configuration](std::string_view ticket) {
          persistence::MySqlAccountStore store(database_configuration);
          return features::authentication::AccountAuthentication{store}
              .ConsumeTicket(ticket);
        };
      }
      std::shared_ptr<features::characters::CharacterStore> characters;
      if (!database_configuration.empty()) {
        characters = std::make_shared<persistence::MySqlCharacterStore>(
            database_configuration);
      }
      sessions::PlayerSession::MenuDataProvider menu;
      sessions::PlayerSession::EquipmentChangeHandler equipment_change;
      sessions::PlayerSession::ShopCatalogProvider shop_catalog;
      sessions::PlayerSession::ShopPurchaseHandler shop_purchase;
      sessions::PlayerSession::CashPickupHandler cash_pickup;
      sessions::PlayerSession::WeaponPresetHandler weapon_preset;
      sessions::PlayerSession::ItemUseHandler item_use;
      if (!database_configuration.empty()) {
        menu = [database_configuration](
                   const features::characters::CharacterRecord& character,
                   std::uint64_t observed_at) {
          persistence::MySqlInventoryStore store(database_configuration);
          auto snapshot = store.LoadMenuSnapshot(character, observed_at);
          if (!snapshot) {
            throw std::runtime_error("Character inventory is unavailable.");
          }
          auto presets = snapshot->presets.CreateNotification(
              character, snapshot->inventory, observed_at);
          auto data = sessions::CreateMenuSnapshot(
              character, std::move(snapshot->inventory), observed_at);
          data.presets = std::move(presets);
          data.cash = std::move(snapshot->cash);
          return data;
        };
        equipment_change =
            [database_configuration](
                const features::characters::CharacterRecord& character,
                std::uint64_t item_identifier, bool equip,
                std::uint64_t observed_at) {
              persistence::MySqlInventoryStore store(database_configuration);
              return store
                  .ChangeEquipment(character, item_identifier, equip,
                                   observed_at)
                  .packets;
            };
        shop_catalog = features::shop::CreateShopCatalogResponse;
        shop_purchase =
            [database_configuration](
                const features::characters::CharacterRecord& character,
                const protocol::ShopPurchaseRequest& request,
                std::uint64_t observed_at) {
              persistence::MySqlInventoryStore store(database_configuration);
              return store.Purchase(character, request, observed_at);
            };
        cash_pickup =
            [database_configuration](
                const features::characters::CharacterRecord& character,
                const protocol::CashInventoryPickupRequest& request,
                std::uint64_t observed_at) {
              persistence::MySqlInventoryStore store(database_configuration);
              return store.Pickup(character, request, observed_at);
            };
        weapon_preset =
            [database_configuration](
                const features::characters::CharacterRecord& character,
                const protocol::WeaponPresetChangeRequest& request,
                std::uint64_t observed_at) {
              persistence::MySqlInventoryStore store(database_configuration);
              return store.ChangeWeaponPreset(character, request, observed_at)
                  .packets;
            };
        item_use = [database_configuration](
                       const features::characters::CharacterRecord& character,
                       const protocol::ItemUseRequest& request,
                       std::uint64_t observed_at) {
          persistence::MySqlInventoryStore store(database_configuration);
          return store.UseItem(character, request, observed_at);
        };
      }
      const auto owned_session = std::make_shared<sessions::PlayerSession>(
          std::move(consume), std::move(characters), std::move(menu),
          std::move(equipment_change), std::move(shop_catalog),
          std::move(shop_purchase), std::move(cash_pickup),
          std::move(weapon_preset), std::move(item_use),
          features::practice::PracticeSession::Clock::now,
          features::practice::PracticeSession::CreateRandomSeed, rooms, plazas);
      network::ConnectionHandlers handlers;
      handlers.on_packet = [owned_session,
                            rules](const protocol::TcpPacket& packet) {
        auto& session = *owned_session;
        const auto before = session.State();
        auto response = session.Process(packet);
        features::rooms::BindMatchRewardProfiles(rules, response);
        if (before != session.State() &&
            session.State() ==
                sessions::PlayerSessionState::kAwaitingCharacterCreation) {
          std::cout << "Local ticket accepted; character creation required."
                    << std::endl;
        }
        if (before != session.State() &&
            session.State() ==
                sessions::PlayerSessionState::kAwaitingCharacterProfile) {
          std::cout
              << (packet.message_id == 3
                      ? "Local ticket accepted; stored character profile pending."
                      : "Character persisted; login profile pending.")
              << std::endl;
          if (session.Character()) {
            std::cout << "CharacterType=" << session.Character()->character_type
                      << " ColorIndex=" << session.Character()->color_index
                      << std::endl;
          }
        }
        if (before != session.State() &&
            (session.State() ==
                 sessions::PlayerSessionState::kLoginProfileSent ||
             session.State() ==
                 sessions::PlayerSessionState::kAwaitingMenuInitialization)) {
          std::cout
              << (packet.message_id == 3
                      ? "Local ticket accepted; stored character login profile sent."
                      : "Persisted character login profile sent.")
              << std::endl;
        }
        if (before != session.State() &&
            session.State() ==
                sessions::PlayerSessionState::kMenuInitializationSent) {
          std::cout << "Owned character menu initialization sent." << std::endl;
        }
        const bool close =
            session.State() == sessions::PlayerSessionState::kRejected;
        return network::PacketHandlingResult{std::move(response), close};
      };
      handlers.on_tick = [owned_session, rules] {
        auto response = owned_session->Poll();
        features::rooms::BindMatchRewardProfiles(rules, response);
        return network::PacketHandlingResult{std::move(response), false};
      };
      return network::ConnectionHeartbeat::Wrap(
          std::move(handlers),
          [owned_session] { return owned_session->AccountId().has_value(); },
          idle_timeout_seconds);
    };
    return network::TcpServer{}.Run(static_cast<std::uint16_t>(port), once,
                                    create_handler, idle_timeout_seconds, {},
                                    {}, listen_address);
  } catch (const std::exception& error) {
    std::cerr << "Server error: " << error.what() << '\n';
    return 1;
  }
}
}  // namespace bubble_fighter::server::application
