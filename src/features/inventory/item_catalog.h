#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_ITEM_CATALOG_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_ITEM_CATALOG_H_

#include <span>

#include "features/inventory/inventory_slot.h"

namespace bubble_fighter::server::features::inventory {
// E059: current Config/ShopItem2.xml, interpreted against 0x00A17CF0.
// Sales/price metadata is not an entitlement or a grant to this account.
struct ItemDefinition {
  std::uint32_t item_id;
  InventoryUsage usage;
  std::uint32_t allowed_characters;
  std::uint32_t required_level;
  std::uint32_t slot_mask;
  std::uint32_t part_mask;
  std::uint32_t secondary_part_mask;
  std::uint32_t socket_mask;
  std::uint32_t function;
  std::uint32_t parameter;
  bool enabled;
};

class ItemCatalog {
 public:
  static std::span<const ItemDefinition> Entries() noexcept;
  static const ItemDefinition* Find(std::uint32_t item_id) noexcept;
};
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_ITEM_CATALOG_H_
