#ifndef BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_MOVEMENT_TIMELINE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_MOVEMENT_TIMELINE_H_
#include <chrono>
#include <deque>

#include "features/battle/combat_geometry.h"

namespace bubble_fighter::server::features::battle {
// Server receipt times only. The unidentified movement ValueAt20 and
// client packed shot time cannot move this clock or authorize a rewind.
// Samples remain reported positions until a movement solver validates them.
class CombatMovementTimeline {
 public:
  using Time = std::chrono::steady_clock::time_point;
  struct Sample {
    Time received;
    CombatVector position;
  };
  static constexpr std::size_t kMaxSamples = 128;
  static constexpr auto kHistory = std::chrono::seconds{5};
  void Record(Time received, const CombatVector& position);
  void Clear() noexcept { samples_.clear(); }
  std::optional<CombatVector> Latest(Time now,
                                     std::chrono::milliseconds max_age) const;
  // No interpolation across an unobserved long gap. 250 ms is a local
  // query bound, not a recovered movement rate or a physics guarantee.
  std::optional<CombatVector> At(
      Time when,
      std::chrono::milliseconds max_gap = std::chrono::milliseconds{250}) const;
  std::size_t Size() const noexcept { return samples_.size(); }

 private:
  std::deque<Sample> samples_;
};
}  // namespace bubble_fighter::server::features::battle

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_COMBAT_MOVEMENT_TIMELINE_H_
