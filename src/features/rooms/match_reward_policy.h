#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_MATCH_REWARD_POLICY_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_MATCH_REWARD_POLICY_H_
#include <array>
#include <cstdint>
#include <filesystem>
#include <istream>

#include "features/characters/character_statistics.h"
#include "features/rooms/result_award_rules.h"

namespace bubble_fighter::server::features::rooms {
// User-authorized LOCAL policy. These numbers are not Nexon's reward table.
struct MatchRewardPolicy {
  std::uint32_t experience_per_level = 1000, maximum_level = 260;
  std::uint32_t completion_experience = 100, completion_lucci = 50;
  std::uint32_t win_experience = 25, win_lucci = 10;
  std::uint32_t pang_experience = 10, pang_lucci = 5;
  std::uint32_t rescue_experience = 5, rescue_lucci = 2;
  ResultAwardRules awards;
  static MatchRewardPolicy Read(std::istream& input);
  static MatchRewardPolicy Load(const std::filesystem::path& path);
  void Validate() const;
  characters::CharacterStatistics Apply(
      const characters::CharacterStatistics& before,
      const std::array<std::uint16_t, 4>& scores, bool won) const;
  std::array<std::uint32_t, 2> ExperienceRange(std::uint32_t experience) const;
  bool operator==(const MatchRewardPolicy&) const = default;
};
}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_MATCH_REWARD_POLICY_H_
