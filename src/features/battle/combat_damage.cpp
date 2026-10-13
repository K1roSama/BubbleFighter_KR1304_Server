#include "features/battle/combat_damage.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace bubble_fighter::server::features::battle {
namespace {
void RequireNonnegative(float value) {
  if (!std::isfinite(value) || value < 0) {
    throw std::invalid_argument("Invalid combat numeric input.");
  }
}
}  // namespace

float CombatDamage::Calculate(float pure_damage,
                              const CombatDamageFactors& factors) {
  RequireNonnegative(pure_damage);
  if (!std::isfinite(factors.attack_adjustment) ||
      !std::isfinite(factors.defense_adjustment)) {
    throw std::invalid_argument("Invalid combat adjustment.");
  }
  RequireNonnegative(factors.critical_multiplier);
  RequireNonnegative(factors.hit_multiplier);
  RequireNonnegative(factors.range_multiplier);
  // Keep native float32 stage boundaries and operation order.
  float value = pure_damage * std::max(0.0f, 1.0f + factors.attack_adjustment);
  value *= 1.0f + std::clamp(factors.defense_adjustment, -1.0f, 0.0f);
  value *= factors.critical_multiplier;
  value *= factors.hit_multiplier;
  value *= factors.range_multiplier;
  RequireNonnegative(value);
  return value;
}

CombatHealthChange CombatDamage::Apply(float current_health, float damage) {
  RequireNonnegative(current_health);
  RequireNonnegative(damage);
  const float remainder = current_health - damage;
  return {current_health, std::max(0.0f, remainder), damage,
          std::max(0.0f, -remainder), remainder <= 0};
}
}  // namespace bubble_fighter::server::features::battle
