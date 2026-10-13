#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_GRANT_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_GRANT_H_

#include "features/inventory/inventory_slot.h"

namespace bubble_fighter::server::features::inventory {
enum class InventoryGrantStatus { kGranted, kAlreadyOwned, kLimitExceeded };

// Applies an already authorized grant. This does not validate a shop offer,
// accept client quantities, debit currency or persist anything.
class InventoryGrant {
 public:
  static InventoryGrantStatus Apply(CharacterInventory& inventory,
                                    std::uint32_t item_id, std::uint32_t amount,
                                    std::uint32_t max_amount,
                                    std::uint64_t observed_at_unix_seconds);
};
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_GRANT_H_
