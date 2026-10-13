#ifndef BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_WEAPON_CATALOG_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_WEAPON_CATALOG_H_
#include <cstdint>
#include <span>

namespace bubble_fighter::server::features::battle {
// BFCLIENT02 43B030: resolved base values, before equipment/mode/skill
// modifiers. A catalog entry does not grant ownership or damage authority.
struct CombatWeaponDefinition {
  std::uint32_t weapon_id, master_id;
  float speed, range, hit_radius, damage;
  std::int32_t fire_cooldown_milliseconds;
  float near_range, near_angle_degrees;
  std::int32_t near_active_milliseconds, near_delay_milliseconds;
  float scatter_size1, scatter_size2;
  std::int32_t scatter_damage1, scatter_damage2;
  float scatter_first_degree, scatter_second_degree;
  std::int32_t scatter_delay_milliseconds;
};

class CombatWeaponCatalog {
 public:
  static std::span<const CombatWeaponDefinition> Entries() noexcept;
  static const CombatWeaponDefinition* Find(std::uint32_t weapon_id) noexcept;
};
}  // namespace bubble_fighter::server::features::battle

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_WEAPON_CATALOG_H_
