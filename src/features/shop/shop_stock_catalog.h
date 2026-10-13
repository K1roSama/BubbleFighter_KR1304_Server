#ifndef BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_STOCK_CATALOG_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_STOCK_CATALOG_H_

#include <cstdint>
#include <span>

namespace bubble_fighter::server::features::shop {
// Fingerprinted client Stock worksheet, not authoritative server offers.
// Availability, sale dates, currencies and purchase rules need a separate
// policy. ResourceListed does not map to ShopPriceRecord::FlagAt12.
struct ShopStockDefinition {
  std::uint32_t stock_id;
  std::uint32_t item_id;
  bool resource_listed;
  std::uint16_t type;
  std::uint32_t duration;
  std::uint32_t show_price;
  std::uint32_t sell_price;
  bool operator==(const ShopStockDefinition&) const = default;
};

class ShopStockCatalog {
 public:
  static std::span<const ShopStockDefinition> Entries() noexcept;
  // User-authorized local offers only for items without any resource Stock.
  static std::span<const ShopStockDefinition> FallbackEntries() noexcept;
  static const ShopStockDefinition* Find(std::uint32_t stock_id) noexcept;
};
}  // namespace bubble_fighter::server::features::shop

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_STOCK_CATALOG_H_
