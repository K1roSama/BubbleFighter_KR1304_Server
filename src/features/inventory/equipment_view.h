#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_EQUIPMENT_VIEW_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_EQUIPMENT_VIEW_H_

#include "features/characters/character_store.h"
#include "features/inventory/inventory_slot.h"
#include "protocol/scenes/update_party_notification.h"

namespace bubble_fighter::server::features::inventory {
class EquipmentView {
 public:
  // Builds ordinary clothing/weapon/current-character state from trusted
  // persistence and the current client catalog. Never grants starter gear.
  static protocol::PartySupplementaryRecord Create(
      const characters::CharacterRecord& character,
      const CharacterInventory& inventory,
      std::uint64_t observed_at_unix_seconds);
};
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_EQUIPMENT_VIEW_H_
