#include "features/shop/shop_stock_catalog.h"

#include <algorithm>
#include <cstdint>
#include <span>

#include "features/shop/generated/shop_fallback_stock_data.h"
#include "features/shop/generated/shop_stock_catalog_data.h"

namespace bubble_fighter::server::features::shop {
std::span<const ShopStockDefinition> ShopStockCatalog::Entries() noexcept {
  return kShopStockCatalogData;
}

std::span<const ShopStockDefinition>
ShopStockCatalog::FallbackEntries() noexcept {
  return kShopFallbackStockData;
}

const ShopStockDefinition* ShopStockCatalog::Find(
    std::uint32_t stock_id) noexcept {
  const auto entries = Entries();
  const auto found =
      std::lower_bound(entries.begin(), entries.end(), stock_id,
                       [](const ShopStockDefinition& entry, std::uint32_t id) {
                         return entry.stock_id < id;
                       });
  if (found != entries.end() && found->stock_id == stock_id) {
    return &*found;
  }
  const auto fallback = FallbackEntries();
  const auto local =
      std::lower_bound(fallback.begin(), fallback.end(), stock_id,
                       [](const ShopStockDefinition& entry, std::uint32_t id) {
                         return entry.stock_id < id;
                       });
  return local != fallback.end() && local->stock_id == stock_id ? &*local
                                                                : nullptr;
}
}  // namespace bubble_fighter::server::features::shop
