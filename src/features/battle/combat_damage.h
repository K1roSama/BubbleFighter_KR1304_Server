#ifndef BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_DAMAGE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_DAMAGE_H_

namespace bubble_fighter::server::features::battle {
// Inputs must be derived from trusted equipment/mode/skill state. These
// are aggregate modifiers, not fields accepted from a gameplay packet.
struct CombatDamageFactors {
  float attack_adjustment = 0;
  float defense_adjustment = 0;
  float critical_multiplier = 1;
  float hit_multiplier = 1;
  float range_multiplier = 1;
};

struct CombatHealthChange {
  float previous_health, health, applied_damage, excess_damage;
  bool depleted;
};

class CombatDamage {
 public:
  // 6F3200: additive attack clamp, defense clamp, then critical.
  // 6F1460 applies the hit and distance multipliers afterwards.
  // Special intermediate reinforcement/shield operations must be resolved
  // by the caller; this arithmetic alone does not implement those effects.
  static float Calculate(float pure_damage, const CombatDamageFactors& factors);
  // 712E70: subtract HP, retain excess for the later state calculation,
  // and normalize depleted HP to zero. Does not guess Bubble vs Down.
  static CombatHealthChange Apply(float current_health, float damage);
};
}  // namespace bubble_fighter::server::features::battle

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_DAMAGE_H_
