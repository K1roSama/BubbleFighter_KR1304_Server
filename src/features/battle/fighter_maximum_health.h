#ifndef BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_FIGHTER_MAXIMUM_HEALTH_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_FIGHTER_MAXIMUM_HEALTH_H_
#include <cstdint>
#include <span>

namespace bubble_fighter::server::features::battle {
struct FighterHealthEquipment {
  float amount = 0;
  float ratio = 0;
  bool operator==(const FighterHealthEquipment&) const = default;
};

class FighterMaximumHealth {
 public:
  // 818AA0 applies the character first and each other item once in ID
  // order. Input is the trusted S117 equipment view, never a client hit.
  static FighterHealthEquipment Equipment(std::uint32_t character_type,
                                          std::span<const std::uint32_t> items,
                                          std::uint32_t channel);
  // 480F30, ordinary mode 1: optional runtime scale precedes equipment;
  // Lua scale follows it. Both scales must come from known server state.
  // This does not evaluate other modes or activate skills by itself.
  static float Ordinary(float base_hp, const FighterHealthEquipment& equipment,
                        float actor_scale, float lua_scale);
};
}  // namespace bubble_fighter::server::features::battle

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_FIGHTER_MAXIMUM_HEALTH_H_
