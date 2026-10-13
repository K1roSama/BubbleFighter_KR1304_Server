#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

#include "features/authentication/account_authentication.h"
#include "features/characters/character_profile.h"
#include "features/configuration/content_config_response.h"
#include "features/inventory/default_inventory.h"
#include "features/rooms/room_game_session.h"
#include "features/rooms/room_match_settlement.h"
#include "features/shop/cash_shop.h"
#include "features/shop/shop_catalog.h"
#include "features/shop/shop_catalog_response.h"
#include "features/shop/shop_purchase.h"
#include "features/shop/shop_stock_catalog.h"
#include "protocol/codec/zlib_stored_stream.h"
#include "protocol/configuration/content_configuration.h"
#include "protocol/login/client_to_server_message_0x0001.h"
#include "protocol/login/login_request.h"
#include "protocol/unresolved/client_to_server_message_0x0209.h"
#include "sessions/menu_snapshot.h"
#include "sessions/player_session.h"

namespace bf = bubble_fighter::server;
namespace auth = bf::features::authentication;
namespace inv = bf::features::inventory;
namespace shop = bf::features::shop;
namespace rooms = bf::features::rooms;
namespace proto = bf::protocol;

namespace {
int checks = 0;
void Check(bool ok, const char* message) {
  ++checks;
  if (!ok) throw std::runtime_error(message);
}
template <typename Operation>
void Throws(Operation operation) {
  bool threw = false;
  try {
    operation();
  } catch (const std::exception&) {
    threw = true;
  }
  Check(threw, "Expected rejection");
}
std::map<std::string, std::string> actual;
void Snapshot(const std::string& name,
              const std::vector<proto::TcpPacket>& packets) {
  std::string hex;
  constexpr char digits[] = "0123456789abcdef";
  for (const auto& packet : packets) {
    for (auto byte : proto::TcpPacketCodec::Encode(packet)) {
      hex += digits[byte >> 4];
      hex += digits[byte & 15];
    }
  }
  actual.emplace(name, std::move(hex));
}
void Ids(const std::vector<proto::TcpPacket>& packets,
         std::initializer_list<std::uint16_t> expected) {
  std::vector<std::uint16_t> ids;
  for (const auto& packet : packets) ids.push_back(packet.message_id);
  Check(ids == std::vector<std::uint16_t>(expected), "Packet order changed");
}
bf::features::characters::CharacterRecord Character() {
  return {7, 3, u"Tester", 10, 7, {1, 0, 1000000, 1000000}};
}
void ProtocolTests() {
  proto::ShopPurchaseRequest request{0x12345678, 0};
  const std::vector<std::uint8_t> payload{0x78, 0x56, 0x34, 0x12, 0, 0, 0, 0};
  Check(request.Encode().payload == payload, "Purchase byte fixture");
  const auto decoded = proto::ShopPurchaseRequest::Decode({809, payload});
  Check(decoded.stock_id == 0x12345678 && decoded.value_at4 == 0,
        "Purchase decode fixture");
  Throws([&] { proto::ShopPurchaseRequest::Decode({814, payload}); });
  Throws([&] { proto::ShopPurchaseRequest::Decode({809, {0}}); });
  auto extra = payload;
  extra.push_back(0);
  Throws([&] { proto::ShopPurchaseRequest::Decode({809, extra}); });
  const std::vector<std::uint8_t> frame{3, 0,    0, 0, 0x34, 0x12,
                                        0, 0x25, 1, 2, 3};
  Check(proto::TcpPacketCodec::Encode({0x1234, {1, 2, 3}}) == frame,
        "TCP byte fixture");
  Check(proto::TcpPacketCodec::Decode(frame).status ==
            proto::PacketDecodeStatus::kComplete,
        "TCP fixture decode");
  auto bad = frame;
  bad[7] ^= 1;
  Check(proto::TcpPacketCodec::Decode(bad).status ==
            proto::PacketDecodeStatus::kInvalidHeaderChecksum,
        "Header checksum");
  bad = frame;
  bad.back() ^= 1;
  Check(proto::TcpPacketCodec::Decode(bad).status ==
            proto::PacketDecodeStatus::kInvalidPayloadChecksum,
        "Payload checksum");
  Check(proto::TcpPacketCodec::Decode(frame, 2).status ==
            proto::PacketDecodeStatus::kPayloadTooLarge,
        "Payload bound");
  Snapshot("default_cash_pickup",
           {proto::CashInventoryPickupResponse{}.Encode()});
  Snapshot("default_purchase", {proto::ShopPurchaseResponse{}.Encode()});
}
void CommerceTests() {
  auto character = Character();
  auto inventory = inv::DefaultInventory::Prepare(character, {3, 7, {}}, 1000);
  auto menu = bf::sessions::CreateMenuSnapshot(character, inventory, 1000);
  Check(menu.profile.fighter.values_at296 ==
            std::array<std::uint32_t, 2>{1000, 1},
        "Fighter baseline");
  Check(menu.profile.fighter.abilities.size() == 6, "Ability baseline");
  Snapshot("profile",
           {bf::features::characters::CharacterProfile::BindKnownFields(
                menu.profile, character, 3)
                .Encode()});
  Snapshot("menu",
           bf::sessions::MenuInitialization::Prepare(character, 3, menu, 1000));
  auto lucci = *shop::ShopStockCatalog::Find(1000001);
  auto cash = *shop::ShopStockCatalog::Find(1000127);
  Snapshot("fixture_offer_ids",
           {{0,
             {static_cast<std::uint8_t>(lucci.stock_id),
              static_cast<std::uint8_t>(lucci.stock_id >> 8),
              static_cast<std::uint8_t>(lucci.stock_id >> 16),
              static_cast<std::uint8_t>(lucci.stock_id >> 24),
              static_cast<std::uint8_t>(cash.stock_id),
              static_cast<std::uint8_t>(cash.stock_id >> 8),
              static_cast<std::uint8_t>(cash.stock_id >> 16),
              static_cast<std::uint8_t>(cash.stock_id >> 24)}}});
  auto buy = shop::ShopPurchase::Prepare({lucci.stock_id, 0}, character,
                                         inventory, 1000);
  Check(buy.response.result == 1 &&
            buy.character.statistics.lucci == 1000000 - lucci.sell_price,
        "Lucci purchase state");
  Ids(buy.packets, {105, 802, 806, 816});
  Snapshot("lucci_success", buy.packets);
  auto broke = character;
  broke.statistics.lucci = 0;
  auto failed =
      shop::ShopPurchase::Prepare({lucci.stock_id, 0}, broke, inventory, 1000);
  Check(failed.response.result == 0xfffffff3U &&
            failed.inventory.slots == inventory.slots,
        "Lucci failure state");
  Ids(failed.packets, {816});
  Snapshot("lucci_insufficient", failed.packets);
  auto invalid =
      shop::ShopPurchase::Prepare({0, 0}, character, inventory, 1000);
  Check(invalid.response.result == 0xffffff38U, "Invalid offer");
  Snapshot("lucci_invalid", invalid.packets);
  auto corrupt = inventory;
  corrupt.account_id = 4;
  Throws(
      [&] { shop::ShopPurchase::Prepare({0, 0}, character, corrupt, 1000); });
  auto owned = shop::ShopPurchase::Prepare({lucci.stock_id, 0}, buy.character,
                                           buy.inventory, 1000);
  Check(owned.response.result == 9 &&
            owned.character.statistics.lucci == buy.character.statistics.lucci,
        "Already owned purchase");
  auto paid = shop::CashShop::Purchase({cash.stock_id, 0}, character, inventory,
                                       {3, 7, {}}, 100, 1000);
  Check(
      paid.purchase.response.result == 1 &&
          paid.purchase.character.statistics.cash == 1000000 - cash.sell_price,
      "Cash purchase state");
  Check(paid.purchase.inventory.slots == inventory.slots &&
            !paid.pending.slots.empty(),
        "Cash pending grant");
  Ids(paid.purchase.packets, {105, 810, 816});
  Snapshot("cash_success", paid.purchase.packets);
  broke = character;
  broke.statistics.cash = 0;
  auto cash_failed = shop::CashShop::Purchase({cash.stock_id, 0}, broke,
                                              inventory, {3, 7, {}}, 100, 1000);
  Check(cash_failed.purchase.response.result == 36 &&
            cash_failed.pending.slots.empty(),
        "Cash insufficient state");
  Ids(cash_failed.purchase.packets, {816});
  Snapshot("cash_insufficient", cash_failed.purchase.packets);
  auto picked = shop::CashShop::Pickup({100}, paid.purchase.character,
                                       inventory, paid.pending, 2000);
  Check(picked.response.result == 1 &&
            picked.pending.slots.size() + 1 == paid.pending.slots.size(),
        "Cash pickup state");
  Ids(picked.packets, {802, 806, 835, 810});
  Snapshot("pickup_success", picked.packets);
  auto missing =
      shop::CashShop::Pickup({999}, character, inventory, paid.pending, 2000);
  Check(missing.response.result == 2 &&
            missing.pending.slots.size() == paid.pending.slots.size(),
        "Missing pickup state");
  Ids(missing.packets, {835, 810});
  Snapshot("pickup_missing", missing.packets);
  auto repeated = shop::CashShop::Pickup({100}, character, picked.inventory,
                                         paid.pending, 2000);
  Check(repeated.response.result == 1 &&
            repeated.pending.slots.size() + 1 == paid.pending.slots.size(),
        "Repeated eligible pickup grant");
  Ids(repeated.packets, {802, 806, 835, 810});
  auto full = paid.pending;
  full.slots.clear();
  for (std::uint32_t id = 1; id <= 1024; ++id) {
    auto slot = paid.pending.slots.front();
    slot.slot_id = id;
    full.slots.push_back(slot);
  }
  auto capacity = shop::CashShop::Purchase({cash.stock_id, 0}, character,
                                           inventory, full, 2000, 1000);
  Check(capacity.purchase.response.result == 0xffffff38U &&
            capacity.purchase.character.statistics.cash ==
                character.statistics.cash,
        "Cash capacity refusal");
  Throws([&] {
    shop::CashShop::Purchase({cash.stock_id, 0}, character, inventory,
                             {3, 7, {}}, 0, 1000);
  });
  auto invalid_character = character;
  invalid_character.statistics.level = 0;
  Throws([&] {
    shop::ShopPurchase::Prepare({0, 0}, invalid_character, inventory, 1000);
  });
}
void ConfigurationTests() {
  proto::DynamicConfigRequest request;
  auto absent =
      bf::features::configuration::RespondContentConfiguration(request);
  Check(!absent.records[proto::ContentConfiguration::kCategory],
        "Absent checksum policy");
  Snapshot("config_absent", {absent.Encode()});
  request.checksums[proto::ContentConfiguration::kCategory] = 0;
  auto changed =
      bf::features::configuration::RespondContentConfiguration(request);
  Check(changed.records[proto::ContentConfiguration::kCategory].has_value(),
        "Config mismatch");
  Snapshot("config_miss", {changed.Encode()});
  request.checksums[proto::ContentConfiguration::kCategory] =
      proto::ZlibStoredStream::Adler32(
          *changed.records[proto::ContentConfiguration::kCategory], 0);
  auto hit = bf::features::configuration::RespondContentConfiguration(request);
  Check(!hit.records[proto::ContentConfiguration::kCategory], "Config hit");
  Snapshot("config_hit", {hit.Encode()});
}
struct AccountFixture : public auth::AccountStore {
 public:
  std::optional<auth::StoredAccount> account;
  auth::CredentialDigest ticket{};
  std::uint32_t now = 0, expiry = 0;
  bool consumed = false;
  bool CreateAccount(std::string_view,
                     const auth::PasswordVerifier& password) override {
    account = auth::StoredAccount{3, password, false};
    return true;
  }
  std::optional<auth::StoredAccount> FindAccount(std::string_view) override {
    return account;
  }
  bool AddTicket(std::uint64_t, const auth::CredentialDigest& digest,
                 std::uint32_t lifetime) override {
    ticket = digest;
    expiry = now + lifetime;
    consumed = false;
    return true;
  }
  std::optional<std::uint64_t> ConsumeTicket(
      const auth::CredentialDigest& digest) override {
    if (consumed || now >= expiry || digest != ticket || account->is_disabled)
      return {};
    consumed = true;
    return 3;
  }
};
void SessionTests() {
  AccountFixture store;
  auth::AccountAuthentication authentication(store);
  Check(authentication.NormalizeUsername("AbC_123") == "abc_123",
        "Username normalization");
  Throws([&] { authentication.NormalizeUsername("ab"); });
  authentication.CreateAccount("tester", "test-password-123");
  Check(!authentication.Login("tester", "wrong"), "Password failure");
  auto ticket = authentication.Login("tester", "test-password-123");
  Check(ticket.has_value() && store.expiry == 300,
        "Ticket lifetime passed to store");
  Check(authentication.ConsumeTicket(*ticket) == 3 &&
            !authentication.ConsumeTicket(*ticket),
        "One-shot ticket contract");
  ticket = authentication.Login("tester", "test-password-123");
  store.now = 300;
  Check(!authentication.ConsumeTicket(*ticket), "Expiry contract");
  bf::sessions::PlayerSession session;
  proto::ClientToServerMessage0x0001 initial;
  initial.initial_value = 1256;
  const auto responses = session.Process(initial.Encode());
  Ids(responses, {2, 17});
  Snapshot("initial_handshake", responses);
  Check(session.State() == bf::sessions::PlayerSessionState::kAwaitingTicket,
        "Player handshake transition");
  Throws([&] { session.Process(initial.Encode()); });
  Check(session.State() == bf::sessions::PlayerSessionState::kRejected,
        "Player rejection transition");
  const std::array<std::uint32_t, 1> slots{0};
  const rooms::RoomGameSession::Time zero{};
  rooms::RoomGameSession room(slots, zero, 123, 1000);
  Check(room.Phase() == rooms::RoomGamePhase::kLoading, "Room initial phase");
  Check(room.Poll(zero + std::chrono::seconds{89}).empty(), "Load deadline");
  Check(room.Poll(zero + std::chrono::seconds{90}).empty() &&
            room.Phase() == rooms::RoomGamePhase::kReturning,
        "Load timeout transition");
  Throws([&] { rooms::RoomGameSession::ActorId(16); });
  rooms::RoomGameSession game(slots, zero, 123, 1000);
  game.RequireSettlement();
  auto loading =
      game.Process(0, proto::GameLoadingCompleteRequest{}.Encode(), zero);
  Check(loading.size() == 2 && game.Phase() == rooms::RoomGamePhase::kCountdown,
        "Loading completion transition");
  Check(game.Process(0, proto::GameLoadingCompleteRequest{}.Encode(), zero)
            .empty(),
        "Identical loading report");
  Check(game.Poll(zero + std::chrono::milliseconds{3999}).empty(),
        "Countdown boundary");
  auto started = game.Poll(zero + std::chrono::milliseconds{4000});
  Check(started.size() == 1 && game.Phase() == rooms::RoomGamePhase::kPlaying,
        "Start transition");
  Check(game.Poll(zero + std::chrono::milliseconds{5000}).size() == 1 &&
            game.Phase() == rooms::RoomGamePhase::kAwaitingReports,
        "Time over transition");
  Check(game.Poll(zero + std::chrono::milliseconds{15000}).size() == 1 &&
            game.Phase() == rooms::RoomGamePhase::kResult,
        "Report timeout transition");
  Check(game.Poll(zero + std::chrono::milliseconds{18000}).empty(),
        "Await durable settlement");
  game.ConfirmSettlement(zero + std::chrono::milliseconds{18000});
  Check(game.Poll(zero + std::chrono::milliseconds{21000}).size() == 1 &&
            game.Phase() == rooms::RoomGamePhase::kResultPresentation,
        "Settlement transition");
  Check(game.Poll(zero + std::chrono::milliseconds{35000}).size() == 1 &&
            game.Phase() == rooms::RoomGamePhase::kReturning,
        "Return transition");
}
struct CharacterFixture final : bf::features::characters::CharacterStore {
  bf::features::characters::CharacterRecord character = Character();
  std::optional<bf::features::characters::CharacterRecord> FindByAccount(
      std::uint64_t) override {
    return character;
  }
  bool NameExists(std::u16string_view) override { return false; }
  bf::features::characters::CharacterCreateResult Create(
      bf::features::characters::CharacterRecord&) override {
    return bf::features::characters::CharacterCreateResult::
        kAccountHasCharacter;
  }
};

void PlayerRoutingTests() {
  // Independent expectations obtained from HEAD headers and original Debug
  // object files retained after the successful pre-refactor build.
  auto characters = std::make_shared<CharacterFixture>();
  auto inventory =
      inv::DefaultInventory::Prepare(characters->character, {3, 7, {}}, 1000);
  const auto catalog =
      shop::CreateShopCatalogResponse({}, characters->character, 1000);
  Check(catalog.catalog_revision == 10020168155133007003ULL &&
            catalog.items.size() == 7069 && catalog.categories.size() == 5 &&
            catalog.selected_category == 300,
        "Historical catalog snapshot revision");
  bf::sessions::PlayerSession player(
      [](std::string_view) { return std::optional<std::uint64_t>{3}; },
      characters,
      [inventory](const auto& character, auto observed) {
        return bf::sessions::CreateMenuSnapshot(character, inventory, observed);
      },
      {}, shop::CreateShopCatalogResponse,
      [inventory](const auto& character, const auto& request, auto observed) {
        return shop::ShopPurchase::Prepare(request, character, inventory,
                                           observed);
      });
  proto::ClientToServerMessage0x0001 initial;
  initial.initial_value = 1256;
  Ids(player.Process(initial.Encode()), {2, 17});
  proto::LoginRequest login;
  login.nexon_passport = u"BFS1." + std::u16string(64, u'0');
  Ids(player.Process(login.Encode()), {3, 201});
  Check(player.State() ==
            bf::sessions::PlayerSessionState::kAwaitingMenuInitialization,
        "Player authenticated transition");
  Ids(player.Process(proto::ClientToServerMessage0x0209{}.Encode()),
      {105, 117, 806, 837, 802});
  Check(player.State() ==
            bf::sessions::PlayerSessionState::kMenuInitializationSent,
        "Player menu transition");
  Ids(player.Process(proto::ShopPurchaseRequest{1000001, 0}.Encode()),
      {105, 802, 806, 816});
  Check(player.Character()->statistics.lucci == 999999,
        "Player committed balance adoption");
  Check(player.Process({0xffff, {}}).empty() &&
            player.State() ==
                bf::sessions::PlayerSessionState::kMenuInitializationSent,
        "Unhandled packet preserves initialized state");
}

void RuleAndDigestTests() {
  const std::array<std::uint8_t, 3> abc{'a', 'b', 'c'};
  const auth::CredentialDigest expected{
      0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40,
      0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17,
      0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
  Check(auth::CredentialCrypto::Sha256(abc) == expected,
        "SHA-256 known answer");
  const std::array<std::uint8_t, 1025> oversized{};
  Throws([&] { auth::CredentialCrypto::Sha256(oversized); });
  rooms::MatchRewardPolicy rules;
  const auto after = rules.Apply({3, 990, 0, 7}, {0, 1, 2, 0}, true);
  Check(after.level == 4 && after.experience == 1135 && after.lucci == 69 &&
            after.cash == 7,
        "Reward and crossed-level baseline");
  Check(rules.ExperienceRange(1135) == std::array<std::uint32_t, 2>{1000, 2000},
        "Experience interval");
  Check(rules.Apply({260, 990, 0, 0}, {}, false).level == 260,
        "Maximum level baseline");
  rules.experience_per_level = 0;
  Throws([&] { rules.Validate(); });
  rooms::RoomMatchRequest request;
  request.match_id[0] = 1;
  request.format_version = 1;
  request.duration_milliseconds = 1000;
  request.team_scores = {1, 0};
  request.members = {{0, 3, 7, 1, {0, 1, 0, 0}}};
  const std::string hex =
      "01000000"
      "01000000000000000000000000000000"
      "e8030000040100006400000032000000190000000a0000000a000000050000000500000002000000"
      "e80300000100000001000000"
      "0000000003000000000000000700000000000000010000010000000000";
  std::vector<std::uint8_t> bytes;
  for (std::size_t index = 0; index < hex.size(); index += 2)
    bytes.push_back(static_cast<std::uint8_t>(
        std::stoul(hex.substr(index, 2), nullptr, 16)));
  Check(request.Encode() == bytes, "Historical V1 settlement bytes");
  Check(rooms::RoomMatchRequest::Decode(bytes).Winner() == 1,
        "Historical settlement decode");
  bytes[0] = 0;
  Throws([&] { rooms::RoomMatchRequest::Decode(bytes); });
}
}  // namespace

int main(int argc, char** argv) {
  try {
    ProtocolTests();
    CommerceTests();
    ConfigurationTests();
    SessionTests();
    RuleAndDigestTests();
    PlayerRoutingTests();
    if (argc != 2) throw std::runtime_error("Expected fixture file path");
    std::ifstream file(argv[1]);
    Check(file.good(), "Fixture file missing");
    std::map<std::string, std::string> expected;
    std::string name, hex;
    while (file >> name >> hex) expected.emplace(name, hex);
    Check(actual == expected, "Frozen response fixtures changed");
    std::cout << checks << " checks passed; " << actual.size()
              << " frozen packet batches\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
