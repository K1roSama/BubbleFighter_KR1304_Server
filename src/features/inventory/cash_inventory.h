#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_CASH_INVENTORY_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_CASH_INVENTORY_H_

#include "features/inventory/inventory_slot.h"
#include "protocol/inventory/cash_inventory_update_notification.h"

namespace bubble_fighter::server::features::inventory {
struct CashInventorySlot {
  std::uint32_t slot_id = 0;
  std::uint32_t stock_id = 0;
  std::uint32_t item_id = 0;
  InventoryUsage usage = InventoryUsage::kCount;
  std::uint32_t amount = 0;
  std::uint32_t max_amount = 0;
  std::uint32_t cash_price = 0;
  std::uint64_t purchased_at_unix_seconds = 0;
  bool operator==(const CashInventorySlot&) const = default;
};

struct CashInventory {
  // Client UI adds 100000000 to cash IDs. Keep the local ID range
  // disjoint from ordinary items and avoid signed arithmetic overflow.
  static constexpr std::uint32_t kMaxSlotId = 99'999'999;
  std::uint64_t account_id = 0;
  std::uint64_t character_id = 0;
  std::vector<CashInventorySlot> slots;
  protocol::CashInventoryUpdateNotification CreateNotification(
      std::uint64_t authenticated_account_id) const;
  bool operator==(const CashInventory&) const = default;
};
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_CASH_INVENTORY_H_
