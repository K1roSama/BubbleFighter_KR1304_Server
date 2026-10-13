#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_VALIDATION_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_VALIDATION_H_

#include "features/characters/character_store.h"
#include "features/inventory/inventory_slot.h"

namespace bubble_fighter::server::features::inventory {

// Predicates let each transaction preserve its validation order and error text.
bool BelongsToCharacter(const characters::CharacterRecord& character,
                        const CharacterInventory& inventory);
bool IsValidInventoryCharacter(const characters::CharacterRecord& character);
bool MatchesItemCatalog(const CharacterInventory& inventory);

}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_VALIDATION_H_
