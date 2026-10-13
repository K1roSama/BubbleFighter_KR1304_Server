#include "features/battle/combat_movement_timeline.h"

#include <stdexcept>

namespace bubble_fighter::server::features::battle {
void CombatMovementTimeline::Record(Time received,
                                    const CombatVector& position) {
  if (!CombatGeometry::IsFinite(position) ||
      (!samples_.empty() && received < samples_.back().received)) {
    throw std::invalid_argument("Invalid combat movement sample.");
  }
  if (!samples_.empty() && received == samples_.back().received) {
    samples_.back().position = position;
    return;
  }
  samples_.push_back({received, position});
  while (samples_.size() > kMaxSamples ||
         samples_.front().received < received - kHistory) {
    samples_.pop_front();
  }
}

std::optional<CombatVector> CombatMovementTimeline::Latest(
    Time now, std::chrono::milliseconds max_age) const {
  if (max_age.count() < 0) {
    throw std::invalid_argument("Negative movement age.");
  }
  if (samples_.empty() || now < samples_.back().received ||
      now - samples_.back().received > max_age) {
    return std::nullopt;
  }
  return samples_.back().position;
}

std::optional<CombatVector> CombatMovementTimeline::At(
    Time when, std::chrono::milliseconds max_gap) const {
  if (max_gap.count() < 0) {
    throw std::invalid_argument("Negative movement interpolation gap.");
  }
  if (samples_.empty() || when < samples_.front().received ||
      when > samples_.back().received) {
    return std::nullopt;
  }
  auto previous = samples_.begin();
  for (auto next = samples_.begin(); next != samples_.end(); ++next) {
    if (next->received == when) {
      return next->position;
    }
    if (next->received > when) {
      if (next->received - previous->received > max_gap) {
        return std::nullopt;
      }
      const double fraction =
          std::chrono::duration<double>(when - previous->received).count() /
          std::chrono::duration<double>(next->received - previous->received)
              .count();
      CombatVector value{};
      for (std::size_t i = 0; i < value.size(); ++i) {
        value[i] = static_cast<float>(
            double(previous->position[i]) +
            fraction * (double(next->position[i]) - previous->position[i]));
      }
      return value;
    }
    previous = next;
  }
  return std::nullopt;
}
}  // namespace bubble_fighter::server::features::battle
