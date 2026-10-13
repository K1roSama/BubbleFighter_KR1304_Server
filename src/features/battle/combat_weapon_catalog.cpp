#include "features/battle/combat_weapon_catalog.h"

#include <algorithm>

#include "features/battle/generated/combat_weapon_catalog_data.h"

namespace bubble_fighter::server::features::battle {
std::span<const CombatWeaponDefinition>
CombatWeaponCatalog::Entries() noexcept {
  return kCombatWeaponCatalogData;
}

const CombatWeaponDefinition* CombatWeaponCatalog::Find(
    std::uint32_t weapon_id) noexcept {
  const auto found = std::lower_bound(
      kCombatWeaponCatalogData.begin(), kCombatWeaponCatalogData.end(),
      weapon_id, [](const CombatWeaponDefinition& entry, std::uint32_t id) {
        return entry.weapon_id < id;
      });
  return found != kCombatWeaponCatalogData.end() &&
                 found->weapon_id == weapon_id
             ? &*found
             : nullptr;
}
}  // namespace bubble_fighter::server::features::battle
