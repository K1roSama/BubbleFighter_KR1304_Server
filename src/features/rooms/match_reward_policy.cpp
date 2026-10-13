#include "features/rooms/match_reward_policy.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>

#include "features/characters/character_statistics.h"

namespace bubble_fighter::server::features::rooms {
namespace {
constexpr auto kMax = characters::CharacterStatistics::kMaxValue;
std::uint32_t Add(std::uint32_t value, std::uint64_t gain) {
  return static_cast<std::uint32_t>(
      std::min<std::uint64_t>(kMax, value + gain));
}
}  // namespace

void MatchRewardPolicy::Validate() const {
  awards.Validate();
  if (experience_per_level == 0 || experience_per_level > kMax ||
      maximum_level == 0 || maximum_level > 260) {
    throw std::invalid_argument("Unsupported local experience progression.");
  }
  for (auto value :
       {completion_experience, completion_lucci, win_experience, win_lucci,
        pang_experience, pang_lucci, rescue_experience, rescue_lucci}) {
    if (value > 1000000) {
      throw std::invalid_argument("Local reward exceeds the configured limit.");
    }
  }
}

characters::CharacterStatistics MatchRewardPolicy::Apply(
    const characters::CharacterStatistics& before,
    const std::array<std::uint16_t, 4>& scores, bool won) const {
  Validate();
  if (!before.IsValid()) {
    throw std::invalid_argument("Invalid reward recipient statistics.");
  }
  auto after = before;
  after.experience =
      Add(before.experience, std::uint64_t{completion_experience} +
                                 (won ? win_experience : 0) +
                                 std::uint64_t{scores[1]} * pang_experience +
                                 std::uint64_t{scores[2]} * rescue_experience);
  after.lucci = Add(before.lucci, std::uint64_t{completion_lucci} +
                                      (won ? win_lucci : 0) +
                                      std::uint64_t{scores[1]} * pang_lucci +
                                      std::uint64_t{scores[2]} * rescue_lucci);
  // Preserve legacy levels even when they do not match cumulative XP.
  // Only thresholds crossed by this award add levels; no retroactive reset.
  if (before.level < maximum_level) {
    const auto crossed = after.experience / experience_per_level -
                         before.experience / experience_per_level;
    after.level = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        maximum_level, std::uint64_t{before.level} + crossed));
  }
  return after;
}

std::array<std::uint32_t, 2> MatchRewardPolicy::ExperienceRange(
    std::uint32_t experience) const {
  Validate();
  if (experience > kMax) {
    throw std::invalid_argument("Experience outside client signed range.");
  }
  const auto base = experience / experience_per_level * experience_per_level;
  return {base, Add(base, experience_per_level)};
}

}  // namespace bubble_fighter::server::features::rooms
