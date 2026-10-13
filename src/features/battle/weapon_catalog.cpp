#include "features/battle/weapon_catalog.h"

#include <algorithm>

#include "features/battle/generated/weapon_catalog_data.h"

namespace bubble_fighter::server::features::battle {
std::span<const WeaponDefinition> WeaponCatalog::Entries() noexcept {
  return kWeaponCatalogData;
}

const WeaponDefinition* WeaponCatalog::Find(std::uint32_t weapon_id) noexcept {
  const auto found = std::lower_bound(
      kWeaponCatalogData.begin(), kWeaponCatalogData.end(), weapon_id,
      [](const WeaponDefinition& entry, std::uint32_t id) {
        return entry.weapon_id < id;
      });
  return found != kWeaponCatalogData.end() && found->weapon_id == weapon_id
             ? &*found
             : nullptr;
}

const WeaponDefinition* WeaponCatalog::FindEquipmentItem(
    std::uint32_t item_id) noexcept {
  return item_id >= 201000 && item_id <= 210999 ? Find(item_id - 200000)
                                                : nullptr;
}
}  // namespace bubble_fighter::server::features::battle
