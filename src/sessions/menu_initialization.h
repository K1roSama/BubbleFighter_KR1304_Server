#ifndef BUBBLEFIGHTER_SERVER_SESSIONS_MENU_INITIALIZATION_H_
#define BUBBLEFIGHTER_SERVER_SESSIONS_MENU_INITIALIZATION_H_

#include <optional>

#include "features/characters/character_store.h"
#include "features/inventory/cash_inventory.h"
#include "features/inventory/inventory_refresh.h"
#include "features/inventory/inventory_slot.h"
#include "protocol/characters/user_stat_notification.h"
#include "protocol/inventory/weapon_preset_update_notification.h"
#include "protocol/scenes/goto_room_notification.h"

namespace bubble_fighter::server::sessions {
// A trusted data source must provide COMPLETE opaque state. This aggregate
// is not a zero-filled new-character template. Known identity/statistics
// fields are bound centrally, while unknown fields remain explicit.
struct MenuInitializationData {
  protocol::UserStatNotification profile;
  protocol::GotoRoomNotification scene;
  protocol::SessionMember member;
  protocol::PartySupplementaryRecord equipment;
  protocol::WeaponPresetUpdateNotification presets;
  features::inventory::CharacterInventory inventory;
  // Optional only for legacy callers; MySQL always supplies a loaded snapshot.
  std::optional<features::inventory::CashInventory> cash;
};

class MenuInitialization {
 public:
  // E054: 0x0081B820 narrows preset count to uint8. This is a safety
  // ceiling, not a recovered number of available preset pages.
  static constexpr std::size_t kMaxPresetRecords =
      features::inventory::InventoryRefresh::kMaxPresetRecords;
  // Builds the ENTIRE ordered batch before handing anything to transport.
  // S3 and the authenticated solo S201 have already been sent.
  static std::vector<protocol::TcpPacket> Prepare(
      const features::characters::CharacterRecord& character,
      std::uint64_t authenticated_account_id, MenuInitializationData data,
      std::uint64_t observed_at_unix_seconds);
};
}  // namespace bubble_fighter::server::sessions

#endif  // BUBBLEFIGHTER_SERVER_SESSIONS_MENU_INITIALIZATION_H_
