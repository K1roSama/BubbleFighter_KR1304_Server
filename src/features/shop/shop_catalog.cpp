#include "features/shop/shop_catalog.h"

#include <algorithm>
#include <cstdint>
#include <span>

#include "features/shop/generated/shop_catalog_data.h"

namespace bubble_fighter::server::features::shop {
std::span<const ShopItemDefinition> ShopCatalog::Items() noexcept {
  return kShopCatalogItems;
}
const ShopItemDefinition* ShopCatalog::FindItem(
    std::uint32_t item_id) noexcept {
  const auto items = Items();
  const auto found = std::lower_bound(
      items.begin(), items.end(), item_id,
      [](const auto& item, std::uint32_t id) { return item.item_id < id; });
  return found != items.end() && found->item_id == item_id ? &*found : nullptr;
}
std::span<const ShopCategoryDefinition> ShopCatalog::Categories() noexcept {
  return kShopCatalogCategories;
}
std::span<const ShopSubCategoryDefinition>
ShopCatalog::SubCategories() noexcept {
  return kShopCatalogSubCategories;
}
}  // namespace bubble_fighter::server::features::shop
