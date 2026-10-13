#include "features/inventory/inventory_validation.h"

#include "features/characters/character_creation.h"
#include "features/inventory/item_catalog.h"

namespace bubble_fighter::server::features::inventory {

bool BelongsToCharacter(const characters::CharacterRecord& character,
                        const CharacterInventory& inventory) {
  return inventory.account_id == character.account_id &&
         inventory.character_id == character.character_id;
}

bool IsValidInventoryCharacter(const characters::CharacterRecord& character) {
  return character.statistics.IsValid() &&
         characters::CharacterCreation::SupportsSelection(
             character.character_type, character.color_index);
}

bool MatchesItemCatalog(const CharacterInventory& inventory) {
  for (const auto& slot : inventory.slots) {
    const auto* item = ItemCatalog::Find(slot.item_id);
    if (!item || item->usage != slot.usage) return false;
  }
  return true;
}

}  // namespace bubble_fighter::server::features::inventory
