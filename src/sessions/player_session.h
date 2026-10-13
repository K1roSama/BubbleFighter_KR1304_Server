#ifndef BUBBLEFIGHTER_SERVER_SESSIONS_PLAYER_SESSION_H_
#define BUBBLEFIGHTER_SERVER_SESSIONS_PLAYER_SESSION_H_

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

#include "features/authentication/initial_handshake.h"
#include "features/characters/character_store.h"
#include "features/chat/scene_chat.h"
#include "features/inventory/item_use.h"
#include "features/lobby/lobby_session.h"
#include "features/lobby/plaza_registry.h"
#include "features/practice/practice_session.h"
#include "features/rooms/room_packet_queue.h"
#include "features/shop/cash_shop.h"
#include "features/shop/shop_purchase.h"
#include "protocol/inventory/weapon_preset_change_request.h"
#include "protocol/shop/shop_entry_request.h"
#include "protocol/shop/shop_entry_response.h"
#include "sessions/menu_initialization.h"

namespace bubble_fighter::server::sessions {
enum class PlayerSessionState {
  kAwaitingInitialRecord,
  kAwaitingTicket,
  kAwaitingCharacterCreation,
  kAwaitingCharacterProfile,
  kLoginProfileSent,
  kAwaitingMenuInitialization,
  kMenuInitializationSent,
  kPracticeLoading,
  kPracticeCountdown,
  kPracticePlaying,
  kPracticeAwaitingReport,
  kPracticeFinishing,
  kPracticeResult,
  kInRoom,
  kRejected
};

class PlayerSession {
 public:
  using TicketConsumer =
      std::function<std::optional<std::uint64_t>(std::string_view)>;
  // Supplies trusted local initial state including loaded inventory.
  // ServerApplication installs CreateMenuSnapshot for configured MySQL;
  // callers without a provider retain the explicitly partial legacy path.
  // One observation time is shared by persistence, view and packet encoding.
  using MenuDataProvider = std::function<MenuInitializationData(
      const features::characters::CharacterRecord&,
      std::uint64_t observed_at_unix_seconds)>;
  using EquipmentChangeHandler = std::function<std::vector<protocol::TcpPacket>(
      const features::characters::CharacterRecord&,
      std::uint64_t item_identifier, bool equip,
      std::uint64_t observed_at_unix_seconds)>;
  using ShopCatalogProvider = std::function<protocol::ShopEntryResponse(
      const protocol::ShopEntryRequest&,
      const features::characters::CharacterRecord&,
      std::uint64_t observed_at_unix_seconds)>;
  using ShopPurchaseHandler = std::function<features::shop::ShopPurchaseResult(
      const features::characters::CharacterRecord&,
      const protocol::ShopPurchaseRequest&,
      std::uint64_t observed_at_unix_seconds)>;
  using CashPickupHandler = std::function<features::shop::CashPickupResult(
      const features::characters::CharacterRecord&,
      const protocol::CashInventoryPickupRequest&,
      std::uint64_t observed_at_unix_seconds)>;
  using WeaponPresetHandler = std::function<std::vector<protocol::TcpPacket>(
      const features::characters::CharacterRecord&,
      const protocol::WeaponPresetChangeRequest&,
      std::uint64_t observed_at_unix_seconds)>;
  using ItemUseHandler = std::function<features::inventory::ItemUseResult(
      const features::characters::CharacterRecord&,
      const protocol::ItemUseRequest&, std::uint64_t observed_at_unix_seconds)>;
  using PracticeClock =
      std::function<features::practice::PracticeSession::Time()>;
  using PracticeSeedFactory = std::function<std::uint32_t()>;
  explicit PlayerSession(
      TicketConsumer consume_ticket = {},
      std::shared_ptr<features::characters::CharacterStore> characters = {},
      MenuDataProvider menu_data = {},
      EquipmentChangeHandler equipment_change = {},
      ShopCatalogProvider shop_catalog = {},
      ShopPurchaseHandler shop_purchase = {},
      CashPickupHandler cash_pickup = {},
      WeaponPresetHandler weapon_preset = {}, ItemUseHandler item_use = {},
      PracticeClock practice_clock =
          features::practice::PracticeSession::Clock::now,
      PracticeSeedFactory practice_seed =
          features::practice::PracticeSession::CreateRandomSeed,
      std::shared_ptr<features::rooms::RoomRegistry> rooms = {},
      std::shared_ptr<features::lobby::PlazaRegistry> plazas = {});
  std::vector<protocol::TcpPacket> Process(const protocol::TcpPacket& packet);
  // Called by the transport even when no packet arrives. Uses monotonic time.
  std::vector<protocol::TcpPacket> Poll();
  PlayerSessionState State() const noexcept;
  std::optional<std::uint64_t> AccountId() const noexcept;
  const std::optional<features::characters::CharacterRecord>& Character()
      const noexcept;

 private:
  static std::uint64_t InventoryObservationTime();
  std::optional<std::vector<protocol::TcpPacket>> ProcessCommerce(
      const protocol::TcpPacket& packet);
  std::optional<std::vector<protocol::TcpPacket>> ProcessPractice(
      const protocol::TcpPacket& packet);
  std::vector<protocol::TcpPacket> ProcessCore(
      const protocol::TcpPacket& packet);
  std::optional<std::vector<protocol::TcpPacket>> ProcessRoom(
      const protocol::TcpPacket& packet);
  std::vector<protocol::TcpPacket> SendLoginProfile();
  std::vector<protocol::TcpPacket> ReturnFromPractice();
  std::vector<protocol::TcpPacket> EnterPlaza(MenuInitializationData data);
  void Reject() noexcept;
  void UpdatePracticeState() noexcept;
  bool CommerceInitialized() const noexcept;
  std::vector<protocol::TcpPacket> ChangeInventory(
      const std::function<std::vector<protocol::TcpPacket>()>& change);
  features::chat::SceneChat chat_;
  features::authentication::InitialHandshake handshake_;
  TicketConsumer consume_ticket_;
  std::shared_ptr<features::characters::CharacterStore> characters_;
  MenuDataProvider menu_data_;
  EquipmentChangeHandler equipment_change_;
  ShopCatalogProvider shop_catalog_;
  ShopPurchaseHandler shop_purchase_;
  CashPickupHandler cash_pickup_;
  WeaponPresetHandler weapon_preset_;
  ItemUseHandler item_use_;
  std::optional<std::uint64_t> shop_revision_sent_;
  PlayerSessionState state_ = PlayerSessionState::kAwaitingInitialRecord;
  std::optional<std::uint64_t> account_id_;
  std::optional<features::characters::CharacterRecord> character_;
  std::u16string checked_name_;
  std::optional<features::lobby::LobbySession> lobby_;
  PracticeClock practice_clock_;
  PracticeSeedFactory practice_seed_;
  std::optional<features::practice::PracticeSession> practice_;
  std::shared_ptr<features::rooms::RoomRegistry> rooms_;
  std::shared_ptr<features::rooms::RoomPacketQueue> room_packets_ =
      std::make_shared<features::rooms::RoomPacketQueue>();
  features::rooms::RoomLease room_;
  std::shared_ptr<features::lobby::PlazaRegistry> plazas_;
  std::shared_ptr<features::rooms::RoomPacketQueue> plaza_packets_ =
      std::make_shared<features::rooms::RoomPacketQueue>();
  features::lobby::PlazaLease plaza_;
};
}  // namespace bubble_fighter::server::sessions

#endif  // BUBBLEFIGHTER_SERVER_SESSIONS_PLAYER_SESSION_H_
