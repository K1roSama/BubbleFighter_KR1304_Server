#include "features/shop/shop_catalog_response.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <stdexcept>
#include <utility>

#include "crypto/sha256.h"
#include "features/characters/character_creation.h"
#include "features/characters/character_store.h"
#include "features/shop/shop_catalog.h"
#include "features/shop/shop_stock_catalog.h"
#include "protocol/shop/shop_entry_request.h"
#include "protocol/shop/shop_entry_response.h"

namespace bubble_fighter::server::features::shop {
namespace {
std::array<std::uint8_t, 32> HashCatalog(std::span<const std::uint8_t> bytes) {
  if (bytes.size() > protocol::ShopEntryResponse::kMaxPayloadBytes) {
    throw std::invalid_argument(
        "Catalog digest input exceeds the payload limit.");
  }
  const auto result = crypto::HashSha256(bytes);
  if (result.status == crypto::Sha256Status::kInitializationFailed) {
    throw std::runtime_error("Cannot initialize catalog digest.");
  }
  if (result.status != crypto::Sha256Status::kSuccess) {
    throw std::runtime_error("Cannot hash catalog snapshot.");
  }
  return result.digest;
}
}  // namespace

protocol::ShopEntryResponse CreateShopCatalogResponse(
    const protocol::ShopEntryRequest& request,
    const characters::CharacterRecord& character,
    std::uint64_t observed_at_unix_seconds) {
  (void)observed_at_unix_seconds;  // Full catalog is independent of historical
                                   // sale windows.
  if (character.account_id == 0 || character.character_id == 0 ||
      !character.statistics.IsValid() ||
      !characters::CharacterCreation::SupportsSelection(
          character.character_type, character.color_index)) {
    throw std::invalid_argument(
        "Shop browsing requires a trusted persisted character.");
  }
  protocol::ShopEntryResponse response;
  // 0x009ADA00 passes (ValueAt0 == 1) to 0x00646F80: the local catalog
  // suppresses the dynamic event-notice popup. This is not a success code.
  response.value_at0 = 1;
  response.refresh_catalog = true;
  // Opaque wide fields and the exclusion list stay at the explicit local
  // zero/empty baseline. Their original server meanings remain unknown;
  // resource timestamps stay in metadata and are never reinterpreted
  // as these wire values. See E062 limitations before extending policy.
  std::map<std::uint32_t, std::size_t> item_indices;
  for (const auto& definition : ShopCatalog::Items()) {
    const auto categories = ShopCatalog::Categories();
    const auto category = std::find_if(
        categories.begin(), categories.end(), [&](const auto& value) {
          return value.identifier == definition.main_category;
        });
    if (category == categories.end() || !category->visible) {
      continue;
    }
    protocol::ShopItemCardRecord card;
    card.identifier = definition.item_id;
    card.kind = 0;  // current ordinary item branch; StockSet bundles need their
                    // own data
    card.name = definition.name;
    card.value_at36 = definition.display_order;
    card.main_category = definition.main_category;
    card.sub_category = definition.sub_category;
    card.flag_at48 = true;  // current client's card visibility gate
    item_indices.emplace(definition.item_id, response.items.size());
    response.items.push_back(std::move(card));
  }
  // Preserve all resource Stock prices/durations, including unlisted ones.
  // The first option is the local representative price, not a recovered
  // original server default. Client preview renders up to three options.
  for (const auto stocks :
       {ShopStockCatalog::Entries(), ShopStockCatalog::FallbackEntries()})
    for (const auto& stock : stocks) {
      const auto item = item_indices.find(stock.item_id);
      if (item == item_indices.end()) {
        continue;
      }
      response.items[item->second].prices.push_back(
          {stock.stock_id, stock.item_id, true, stock.type, stock.duration,
           stock.show_price, stock.sell_price});
    }
  for (auto& item : response.items) {
    if (item.prices.empty()) {
      throw std::runtime_error("Published item has no price options.");
    }
    item.price = item.prices.front();
  }
  for (const auto& definition : ShopCatalog::Categories()) {
    if (!definition.visible ||
        std::none_of(response.items.begin(), response.items.end(),
                     [&](const auto& item) {
                       return item.main_category == definition.identifier;
                     })) {
      continue;
    }
    protocol::ShopMainCategoryRecord category;
    category.identifier = definition.identifier;
    category.name = definition.name;
    category.display_order = definition.display_order;
    for (const auto& child : ShopCatalog::SubCategories()) {
      if (child.parent_category == definition.identifier && child.visible) {
        category.sub_categories.push_back(
            {child.identifier, std::u16string(child.name), child.display_order,
             child.page_type});
      }
    }
    if (category.sub_categories.empty()) {
      throw std::runtime_error("Published category has no visible pages.");
    }
    response.categories.push_back(std::move(category));
  }
  // Content revision excludes the requested selection and wall clock.
  // It changes when offers, prices, labels or page data change.
  // SHA-256's first eight bytes are a cache hint, never authorization.
  const auto hash = HashCatalog(response.Encode().payload);
  for (std::size_t index = 0; index < 8; ++index) {
    response.catalog_revision |= static_cast<std::uint64_t>(hash[index])
                                 << (8 * index);
  }
  if (response.catalog_revision == 0) {
    response.catalog_revision = 1;
  }
  if (!response.categories.empty()) {
    const auto requested =
        std::find_if(response.categories.begin(), response.categories.end(),
                     [&](const auto& category) {
                       return category.identifier == request.selected_category;
                     });
    const auto fallback =
        std::min_element(response.categories.begin(), response.categories.end(),
                         [](const auto& a, const auto& b) {
                           return a.display_order < b.display_order;
                         });
    response.selected_category =
        (requested != response.categories.end() ? requested : fallback)
            ->identifier;
  }
  return response;
}
}  // namespace bubble_fighter::server::features::shop
