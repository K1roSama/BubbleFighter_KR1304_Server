#ifndef BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_SCENE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_SCENE_H_
#include <span>
#include <string_view>
#include <vector>

#include "features/battle/combat_geometry.h"

namespace bubble_fighter::server::features::battle {
enum class CombatSceneQuery : std::uint8_t {
  kBullet = 1,
  kGrenade = 2,
  kActor = 4
};
struct CombatSceneFace {
  CombatTriangle triangle;
  std::uint8_t queries;
};

// Trusted static resource subset. A missing obstruction is not evidence
// that conditional/dynamic objects or actor sweeps have been simulated.
class CombatScene {
 public:
  explicit CombatScene(std::span<const CombatSceneFace> faces);
  std::optional<double> FirstObstruction(const CombatVector& start,
                                         const CombatVector& end,
                                         CombatSceneQuery query) const;
  std::size_t FaceCount() const noexcept { return faces_.size(); }
  static const CombatScene* Find(std::u16string_view map_key);

 private:
  struct Face {
    CombatSceneFace source;
    CombatVector minimum, maximum;
  };
  std::vector<Face> faces_;
};
}  // namespace bubble_fighter::server::features::battle

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_SCENE_H_
