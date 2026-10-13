#include "features/inventory/inventory_presentation.h"

#include <limits>
#include <stdexcept>

namespace bubble_fighter::server::features::inventory {

protocol::PartySupplementaryRecord CreateInventoryEquipment(
    const characters::CharacterRecord& character,
    const CharacterInventory& inventory,
    std::uint64_t observed_at_unix_seconds) {
  if (character.account_id == 0 || character.character_id == 0 ||
      character.character_id > std::numeric_limits<std::uint32_t>::max() ||
      inventory.account_id != character.account_id ||
      inventory.character_id != character.character_id) {
    throw std::invalid_argument("Local menu inventory ownership is invalid.");
  }
  return EquipmentView::Create(character, inventory, observed_at_unix_seconds);
}

}  // namespace bubble_fighter::server::features::inventory
