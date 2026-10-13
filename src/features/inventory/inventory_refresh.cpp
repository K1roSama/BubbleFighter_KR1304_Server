#include "features/inventory/inventory_refresh.h"

#include <stdexcept>

#include "features/scenes/scene_roster.h"

namespace bubble_fighter::server::features::inventory {
std::vector<protocol::TcpPacket> InventoryRefresh::Prepare(
    const characters::CharacterRecord& character,
    std::uint64_t authenticated_account_id, const CharacterInventory& inventory,
    const protocol::PartySupplementaryRecord& equipment,
    const protocol::WeaponPresetUpdateNotification& presets,
    std::uint64_t observed_at_unix_seconds) {
  if (inventory.account_id != authenticated_account_id ||
      inventory.character_id != character.character_id ||
      equipment.key != character.character_id ||
      presets.player_id != character.character_id ||
      presets.records.size() > kMaxPresetRecords) {
    throw std::invalid_argument(
        "Inventory refresh has inconsistent ownership or presets.");
  }

  const auto view = scenes::SceneRoster::CreateInventoryView(
      authenticated_account_id, character, equipment);
  const auto bag = inventory.CreateNotification(authenticated_account_id,
                                                observed_at_unix_seconds);
  // Local refresh ordering: apply inventory before the final view update.
  // S802 updates the bag/UI; S837 uses the established player's view;
  // S806 refreshes the inventory preview through 0x009AF2F0/0x005F50F0.
  // No S3/S117: reading the bag must not clear identity or switch stages.
  std::vector<protocol::TcpPacket> packets;
  packets.reserve(3);
  packets.push_back(bag.Encode());
  packets.push_back(presets.Encode());
  packets.push_back(view.Encode());
  return packets;
}
}  // namespace bubble_fighter::server::features::inventory
