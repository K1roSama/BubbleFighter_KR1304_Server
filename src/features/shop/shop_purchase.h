#ifndef BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_PURCHASE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_PURCHASE_H_

#include "features/characters/character_store.h"
#include "features/inventory/inventory_slot.h"
#include "protocol/shop/shop_purchase_request.h"
#include "protocol/shop/shop_purchase_response.h"

namespace bubble_fighter::server::features::shop {
struct ShopPurchaseResult {
  characters::CharacterRecord character;
  inventory::CharacterInventory inventory;
  protocol::ShopPurchaseResponse response;
  std::vector<protocol::TcpPacket> packets;
  std::uint32_t lucci_cost = 0;
  std::uint32_t changed_item_id = 0;
};

class ShopPurchase {
 public:
  // Pure transaction preparation. Current support: published ordinary
  // Lucci items with known count/day usage. Cash pickup, bundles and
  // special functions require their own grant flows before acceptance.
  // Storage must supply locked, current balances/inventory and commit
  // both changes atomically only after all packets have been encoded.
  static ShopPurchaseResult Prepare(
      const protocol::ShopPurchaseRequest& request,
      const characters::CharacterRecord& character,
      inventory::CharacterInventory inventory,
      std::uint64_t observed_at_unix_seconds);
};
}  // namespace bubble_fighter::server::features::shop

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_PURCHASE_H_
