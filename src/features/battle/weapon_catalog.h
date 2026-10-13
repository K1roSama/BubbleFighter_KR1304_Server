#ifndef BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_WEAPON_CATALOG_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_WEAPON_CATALOG_H_

#include <cstdint>
#include <span>

namespace bubble_fighter::server::features::battle {
// E088: base resource values only. Runtime reinforcement, alternate fire,
// training-task equipment and inventory ownership are separate concerns.
struct WeaponDefinition {
  std::uint32_t weapon_id;
  std::uint32_t master_id;
  std::uint32_t weapon_slot_index;
  std::uint32_t spread_count;
  bool is_spread;

  bool operator==(const WeaponDefinition&) const = default;
};

class WeaponCatalog {
 public:
  static std::span<const WeaponDefinition> Entries() noexcept;
  static const WeaponDefinition* Find(std::uint32_t weapon_id) noexcept;
  // Native training equipment conversion: item in [201000, 210999]
  // maps to resource item - 200000. A match does not grant the item.
  static const WeaponDefinition* FindEquipmentItem(
      std::uint32_t item_id) noexcept;
};
}  // namespace bubble_fighter::server::features::battle

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_WEAPON_CATALOG_H_
