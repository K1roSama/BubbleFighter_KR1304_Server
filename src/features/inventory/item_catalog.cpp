#include "features/inventory/item_catalog.h"

#include <algorithm>

#include "features/inventory/generated/item_catalog_data.h"

namespace bubble_fighter::server::features::inventory {
std::span<const ItemDefinition> ItemCatalog::Entries() noexcept {
  return kItemCatalogData;
}

const ItemDefinition* ItemCatalog::Find(std::uint32_t item_id) noexcept {
  const auto found = std::lower_bound(
      kItemCatalogData.begin(), kItemCatalogData.end(), item_id,
      [](const ItemDefinition& definition, std::uint32_t id) {
        return definition.item_id < id;
      });
  return found != kItemCatalogData.end() && found->item_id == item_id ? &*found
                                                                      : nullptr;
}
}  // namespace bubble_fighter::server::features::inventory
