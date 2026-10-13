#ifndef BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_CATALOG_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_CATALOG_H_

#include <cstdint>
#include <span>
#include <string_view>

namespace bubble_fighter::server::features::shop {
enum class ShopCurrency { kCash, kLucci, kCashCn };

struct ShopItemDefinition {
  std::uint32_t item_id;
  std::u16string_view name;
  std::uint32_t main_category;
  std::uint32_t sub_category;
  std::uint32_t display_order;
  ShopCurrency currency;
  // Historical resource sale dates retained as UTC metadata. The full
  // local shop does not filter by them. These are NOT opaque wire fields.
  std::uint64_t available_from;
  std::uint64_t available_until;
  // E063 resource maxDuration; count/day unit follows ItemDefinition::Usage.
  std::uint32_t max_amount;
  // Preserve the signed resource value; it is not a boolean.
  std::int32_t buy_again;
};

struct ShopCategoryDefinition {
  std::uint32_t identifier;
  std::u16string_view name;
  std::uint32_t display_order;
  bool visible;
};

struct ShopSubCategoryDefinition {
  std::uint32_t identifier;
  std::uint32_t parent_category;
  std::u16string_view name;
  std::uint32_t display_order;
  bool visible;
  std::uint32_t page_type;
};

class ShopCatalog {
 public:
  static std::span<const ShopItemDefinition> Items() noexcept;
  static const ShopItemDefinition* FindItem(std::uint32_t item_id) noexcept;
  static std::span<const ShopCategoryDefinition> Categories() noexcept;
  static std::span<const ShopSubCategoryDefinition> SubCategories() noexcept;
};
}  // namespace bubble_fighter::server::features::shop

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_CATALOG_H_
