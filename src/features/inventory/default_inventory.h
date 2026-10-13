#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_DEFAULT_INVENTORY_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_DEFAULT_INVENTORY_H_

#include "features/inventory/equipment_view.h"

namespace bubble_fighter::server::features::inventory {
class DefaultInventory {
 public:
  // Local permanent starter policy: four verified native empty-slot
  // weapons plus the owned current character. Preserve existing records.
  static CharacterInventory Prepare(
      const characters::CharacterRecord& character,
      CharacterInventory inventory, std::uint64_t observed_at_unix_seconds);
};
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_DEFAULT_INVENTORY_H_
