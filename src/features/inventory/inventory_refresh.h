#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_REFRESH_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_REFRESH_H_

#include "features/characters/character_store.h"
#include "features/inventory/inventory_slot.h"
#include "protocol/inventory/weapon_preset_update_notification.h"
#include "protocol/scenes/update_party_notification.h"

namespace bubble_fighter::server::features::inventory {
class InventoryRefresh {
 public:
  // 0x0081B820 narrows the count to uint8. This is a safety ceiling,
  // not a recovered entitlement to 255 preset pages.
  static constexpr std::size_t kMaxPresetRecords = 255;

  // Requires an established menu/plaza context. A successful trusted
  // load may be empty; a failed load must never become an empty bag.
  // Encodes the entire batch before the caller can publish any packet.
  static std::vector<protocol::TcpPacket> Prepare(
      const characters::CharacterRecord& character,
      std::uint64_t authenticated_account_id,
      const CharacterInventory& inventory,
      const protocol::PartySupplementaryRecord& equipment,
      const protocol::WeaponPresetUpdateNotification& presets,
      std::uint64_t observed_at_unix_seconds);
};
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_REFRESH_H_
