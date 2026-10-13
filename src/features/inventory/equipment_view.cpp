#include "features/inventory/equipment_view.h"

#include <algorithm>
#include <stdexcept>

#include "features/characters/character_creation.h"
#include "features/inventory/item_catalog.h"

namespace bubble_fighter::server::features::inventory {
protocol::PartySupplementaryRecord EquipmentView::Create(
    const characters::CharacterRecord& character,
    const CharacterInventory& inventory,
    std::uint64_t observed_at_unix_seconds) {
  if (inventory.account_id != character.account_id ||
      inventory.character_id != character.character_id ||
      !characters::CharacterCreation::SupportsSelection(
          character.character_type, character.color_index) ||
      !character.statistics.IsValid()) {
    throw std::invalid_argument(
        "Equipment inventory does not match the persisted character.");
  }
  // This validates ownership, IDs, duplicates, expiry representation and
  // the cumulative inventory budget before a view can escape.
  const auto records =
      inventory.ToRecords(character.account_id, observed_at_unix_seconds);
  protocol::PartySupplementaryRecord view;
  view.key = static_cast<std::uint32_t>(character.character_id);
  view.value = 300000U + character.character_type;
  std::uint32_t occupied_slots = 0;
  std::uint32_t occupied_parts = 0;
  for (std::size_t index = 0; index != inventory.slots.size(); ++index) {
    const auto& slot = inventory.slots[index];
    const auto* definition = ItemCatalog::Find(slot.item_id);
    if (!definition || definition->usage != slot.usage) {
      throw std::invalid_argument(
          "Inventory item is absent from the current catalog or has a mismatched usage.");
    }
    if (!slot.equipped) {
      continue;
    }
    // Explicit local policy for the currently supported ordinary path.
    // Special functions, pets, titles and ArticleSlot instances need
    // their own state; never silently render them as ordinary gear.
    if (!definition->enabled || definition->function != 0 ||
        slot.item_id < 100000 || slot.item_id >= 301000 ||
        (records[index].flags & 2) != 0 ||
        character.statistics.level < definition->required_level ||
        (definition->allowed_characters &
         (std::uint32_t{1} << character.character_type)) == 0) {
      throw std::invalid_argument(
          "Equipped item is unsupported, expired or ineligible for this character.");
    }
    const bool is_character = slot.item_id >= 300000;
    if (is_character && slot.item_id != view.value) {
      throw std::invalid_argument(
          "Equipped character differs from the persisted current character.");
    }
    const auto parts = definition->part_mask | definition->secondary_part_mask;
    if ((!is_character && definition->slot_mask == 0 && parts == 0) ||
        (occupied_slots & definition->slot_mask) != 0 ||
        (occupied_parts & parts) != 0) {
      throw std::invalid_argument(
          "Equipped items have unknown or conflicting positions.");
    }
    occupied_slots |= definition->slot_mask;
    occupied_parts |= parts;
    // 0x00817DF0 consumes the first uint32 vector after the model ID,
    // invoking 0x00817680 for each catalog item. S117 and S806 reuse it.
    view.groups[0].push_back(slot.item_id);
  }
  std::sort(view.groups[0].begin(), view.groups[0].end());
  return view;
}
}  // namespace bubble_fighter::server::features::inventory
