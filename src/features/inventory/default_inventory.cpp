#include "features/inventory/default_inventory.h"

#include <algorithm>
#include <array>
#include <stdexcept>

#include "features/inventory/inventory_grant.h"
#include "features/inventory/item_catalog.h"

namespace bubble_fighter::server::features::inventory {
CharacterInventory DefaultInventory::Prepare(
    const characters::CharacterRecord& character, CharacterInventory inventory,
    std::uint64_t observed_at_unix_seconds) {
  (void)EquipmentView::Create(character, inventory, observed_at_unix_seconds);
  std::uint32_t occupied_slots = 0, occupied_parts = 0;
  for (const auto& slot : inventory.slots) {
    if (!slot.equipped) {
      continue;
    }
    const auto& item = *ItemCatalog::Find(slot.item_id);  // validated above
    occupied_slots |= item.slot_mask;
    occupied_parts |= item.part_mask | item.secondary_part_mask;
  }
  // 0x010EAB10 selectors 0..3, initialized by 0x010EA690.
  // Permanent ownership is an explicit local policy, not an inferred
  // original-server reward or a grant of the entire resource catalog.
  const auto character_item = 300000U + character.character_type;
  const std::array<std::uint32_t, 5> defaults{202006, 201005, 207001, 205033,
                                              character_item};
  for (const auto id : defaults) {
    if (std::any_of(inventory.slots.begin(), inventory.slots.end(),
                    [id](const auto& slot) { return slot.item_id == id; })) {
      continue;
    }
    const auto* definition = ItemCatalog::Find(id);
    if (!definition || definition->usage != InventoryUsage::kDuration ||
        (definition->slot_mask == 0 && id != character_item)) {
      throw std::logic_error("Default inventory catalog changed.");
    }
    if (InventoryGrant::Apply(inventory, id, 0xFFFFFFFFU, 0xFFFFFFFFU,
                              observed_at_unix_seconds) !=
        InventoryGrantStatus::kGranted) {
      throw std::logic_error("Default weapon grant failed.");
    }
    auto& added = inventory.slots.back();
    const auto parts = definition->part_mask | definition->secondary_part_mask;
    added.equipped = (occupied_slots & definition->slot_mask) == 0 &&
                     (occupied_parts & parts) == 0;
    if (added.equipped) {
      occupied_slots |= definition->slot_mask;
      occupied_parts |= parts;
    }
  }
  // Reject invalid eligibility/aggregate size before any database write.
  (void)EquipmentView::Create(character, inventory, observed_at_unix_seconds);
  (void)inventory
      .CreateNotification(character.account_id, observed_at_unix_seconds)
      .Encode();
  return inventory;
}
}  // namespace bubble_fighter::server::features::inventory
