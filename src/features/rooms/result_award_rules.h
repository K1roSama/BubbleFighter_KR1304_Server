#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_RESULT_AWARD_RULES_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_RESULT_AWARD_RULES_H_
#include <array>
#include <cstdint>

namespace bubble_fighter::server::features::rooms {
struct ResultAward {
  std::uint32_t contribution = 0;
  std::uint32_t grade = 8;  // Rank_list.smf: SSS, SS, S, A, B, C, D, E, F.
  std::uint32_t medal =
      0;  // Relative mode-1 BF_title medal ID; zero means none.
  bool operator==(const ResultAward&) const = default;
};

// User-authorized local conditions, not recovered official server thresholds.
struct ResultAwardRules {
  // Minimum contribution for SSS through E; F is the remaining range.
  std::array<std::uint32_t, 8> grade_minimums{60, 40, 30, 20, 10, 5, 3, 1};
  // Bubble, pang, rescue; each row is beginner, expert, master.
  std::array<std::array<std::uint32_t, 3>, 3> medal_minimums{
      {{3, 10, 20}, {3, 10, 20}, {3, 10, 20}}};
  void Validate() const;
  ResultAward Evaluate(const std::array<std::uint16_t, 4>& scores) const;
  bool operator==(const ResultAwardRules&) const = default;
};
}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_RESULT_AWARD_RULES_H_
