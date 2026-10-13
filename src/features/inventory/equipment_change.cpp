#include "features/inventory/equipment_change.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "features/inventory/item_catalog.h"
#include "protocol/inventory/inventory_view_update_notification.h"

namespace bubble_fighter::server::features::inventory {
EquipmentChangeResult EquipmentChange::Prepare(
    const characters::CharacterRecord& character, CharacterInventory inventory,
    std::uint64_t item_identifier, bool equip,
    std::uint64_t observed_at_unix_seconds) {
  if (inventory.account_id != character.account_id ||
      inventory.character_id != character.character_id ||
      item_identifier < 100000 || item_identifier >= 300000) {
    throw std::invalid_argument(
        "Equipment request is outside the owned ordinary clothing/weapon path.");
  }
  (void)inventory.ToRecords(character.account_id, observed_at_unix_seconds);
  const auto item_id = static_cast<std::uint32_t>(item_identifier);
  const auto target =
      std::find_if(inventory.slots.begin(), inventory.slots.end(),
                   [&](const auto& slot) { return slot.item_id == item_id; });
  const auto* definition = ItemCatalog::Find(item_id);
  if (target == inventory.slots.end() || !definition ||
      definition->function != 0 || definition->usage != target->usage) {
    throw std::invalid_argument(
        "Equipment request does not identify an owned supported item.");
  }
  if (equip) {
    // Explicit local replacement policy. The masks come from the
    // current catalog; the original server's conflict policy is unknown.
    for (auto& slot : inventory.slots) {
      if (!slot.equipped || slot.item_id == item_id) {
        continue;
      }
      const auto* other = ItemCatalog::Find(slot.item_id);
      if (!other || other->usage != slot.usage) {
        throw std::invalid_argument(
            "Existing equipment has an unknown catalog definition.");
      }
      if ((other->slot_mask & definition->slot_mask) != 0 ||
          ((other->part_mask | other->secondary_part_mask) &
           (definition->part_mask | definition->secondary_part_mask)) != 0) {
        slot.equipped = false;
      }
    }
  }
  target->equipped = equip;
  // Validation of the final state also allows an expired or disabled
  // ordinary item to be removed without granting it a new usable period.
  const auto view =
      EquipmentView::Create(character, inventory, observed_at_unix_seconds);
  std::vector<protocol::TcpPacket> packets;
  packets.reserve(2);
  packets.push_back(
      inventory
          .CreateNotification(character.account_id, observed_at_unix_seconds)
          .Encode());
  packets.push_back(protocol::InventoryViewUpdateNotification{view}.Encode());
  return {std::move(inventory), std::move(packets)};
}
}  // namespace bubble_fighter::server::features::inventory
