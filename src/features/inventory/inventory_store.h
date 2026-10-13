#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_STORE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_STORE_H_

#include "features/inventory/equipment_change.h"
#include "features/inventory/inventory_slot.h"

namespace bubble_fighter::server::features::inventory {
class InventoryStore {
 public:
  virtual ~InventoryStore() = default;
  // null = missing/disabled/foreign character, present empty = loaded empty.
  // Database failures throw; they must never be treated as an empty bag.
  virtual std::optional<CharacterInventory> Load(
      std::uint64_t authenticated_account_id, std::uint64_t character_id,
      std::uint64_t observed_at_unix_seconds) = 0;
  // Trusted server callers only, not a client grant API. Revision 0 inserts,
  // otherwise compare-and-swap; false means unavailable ownership or stale
  // revision. Publish the new revision only after committing the write.
  virtual bool Save(std::uint64_t authenticated_account_id,
                    std::uint64_t character_id, InventorySlot& slot,
                    std::uint64_t observed_at_unix_seconds) = 0;
  // Locks the owner, reloads all rows, prepares the complete wire update,
  // then commits every changed flag together. No grants or partial save.
  virtual EquipmentChangeResult ChangeEquipment(
      const characters::CharacterRecord& character,
      std::uint64_t item_identifier, bool equip,
      std::uint64_t observed_at_unix_seconds) = 0;
};
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_STORE_H_
