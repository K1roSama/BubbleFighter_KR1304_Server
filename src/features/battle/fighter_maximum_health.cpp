#include "features/battle/fighter_maximum_health.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

#include "features/battle/generated/fighter_health_catalog_data.h"

namespace bubble_fighter::server::features::battle {
FighterHealthEquipment FighterMaximumHealth::Equipment(
    std::uint32_t character_type, std::span<const std::uint32_t> items,
    std::uint32_t channel) {
  if (character_type >= 1000) {
    throw std::invalid_argument("Invalid fighter character key.");
  }
  FighterHealthEquipment result;
  const auto apply = [&](std::uint32_t item) {
    for (const auto& effect : kFighterHealthEffects) {
      if (effect.item_id != item ||
          (effect.channel != 0 && effect.channel != channel)) {
        continue;
      }
      if (effect.kind == 0) {
        result.amount += std::clamp(effect.value, 0.0F, 100.0F);
      } else {
        result.ratio += std::clamp(effect.value, 0.0F, 0.99F);
      }
    }
  };
  apply(300000U + character_type);
  const std::set<std::uint32_t> unique(items.begin(), items.end());
  for (auto item : unique) {
    if (item < 300000U || item >= 301000U) {
      apply(item);
    }
  }
  return result;
}

float FighterMaximumHealth::Ordinary(float base_hp,
                                     const FighterHealthEquipment& equipment,
                                     float actor_scale, float lua_scale) {
  for (float value :
       {base_hp, equipment.amount, equipment.ratio, actor_scale, lua_scale}) {
    if (!std::isfinite(value)) {
      throw std::invalid_argument("Non-finite fighter health input.");
    }
  }
  if (base_hp <= 0 || actor_scale <= 0 || lua_scale <= 0) {
    throw std::invalid_argument("Non-positive fighter health scale.");
  }
  float hp = base_hp * actor_scale;
  // Native aggregate guards IGNORE an out-of-range sum. These are not
  // the individual-item clamps in A04850; preserve both stages.
  if (equipment.amount >= 0 && equipment.amount <= 100.01200103759766F) {
    hp += equipment.amount;
  }
  if (equipment.ratio >= 0 && equipment.ratio <= 0.9900000095367432F) {
    hp *= 1.0F + equipment.ratio;
  }
  hp *= lua_scale;
  if (!std::isfinite(hp) || hp <= 0) {
    throw std::overflow_error("Invalid fighter maximum health.");
  }
  return hp;
}
}  // namespace bubble_fighter::server::features::battle
