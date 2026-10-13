#ifndef BUBBLEFIGHTER_SERVER_FEATURES_SHOP_CASH_SHOP_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_SHOP_CASH_SHOP_H_

#include "features/inventory/cash_inventory.h"
#include "features/shop/shop_purchase.h"
#include "protocol/inventory/cash_inventory_pickup_request.h"
#include "protocol/inventory/cash_inventory_pickup_response.h"

namespace bubble_fighter::server::features::shop {
struct CashPurchaseResult {
  ShopPurchaseResult purchase;
  inventory::CashInventory pending;
  std::uint32_t cash_cost = 0;
};

struct CashPickupResult {
  characters::CharacterRecord character;
  inventory::CharacterInventory inventory;
  inventory::CashInventory pending;
  protocol::CashInventoryPickupResponse response;
  std::uint32_t changed_item_id = 0;
  std::vector<protocol::TcpPacket> packets;
};

class CashShop {
 public:
  // assignedSlotId is allocated by trusted persistence, never by C809.
  static CashPurchaseResult Purchase(
      const protocol::ShopPurchaseRequest& request,
      const characters::CharacterRecord& character,
      const inventory::CharacterInventory& inventory,
      inventory::CashInventory pending, std::uint32_t assigned_slot_id,
      std::uint64_t observed_at_unix_seconds);
  static CashPickupResult Pickup(
      const protocol::CashInventoryPickupRequest& request,
      const characters::CharacterRecord& character,
      inventory::CharacterInventory inventory, inventory::CashInventory pending,
      std::uint64_t observed_at_unix_seconds);
};
}  // namespace bubble_fighter::server::features::shop

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_SHOP_CASH_SHOP_H_
