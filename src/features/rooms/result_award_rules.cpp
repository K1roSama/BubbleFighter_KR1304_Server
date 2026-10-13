#include "features/rooms/result_award_rules.h"

#include <stdexcept>
#include <tuple>

namespace bubble_fighter::server::features::rooms {
void ResultAwardRules::Validate() const {
  std::uint32_t previous =
      327676;  // Maximum accepted contribution is 5 * UINT16_MAX.
  for (const auto minimum : grade_minimums) {
    if (minimum == 0 || minimum >= previous) {
      throw std::invalid_argument(
          "Result grade thresholds must strictly decrease within 1..327675.");
    }
    previous = minimum;
  }
  for (const auto& row : medal_minimums) {
    previous = 0;
    for (const auto minimum : row) {
      if (minimum <= previous || minimum > 65535) {
        throw std::invalid_argument(
            "Result medal thresholds must strictly increase within 1..65535.");
      }
      previous = minimum;
    }
  }
}

ResultAward ResultAwardRules::Evaluate(
    const std::array<std::uint16_t, 4>& scores) const {
  Validate();
  ResultAward result;
  result.contribution =
      std::uint32_t{scores[0]} + 2U * scores[1] + 2U * scores[2];
  for (std::size_t grade = 0; grade < grade_minimums.size(); ++grade) {
    if (result.contribution >= grade_minimums[grade]) {
      result.grade = static_cast<std::uint32_t>(grade);
      break;
    }
  }
  // BF_title.xml mode 1: 10..12 beginners, 4..6 experts, 7..9 masters.
  constexpr std::array<std::array<std::uint32_t, 3>, 3> medals{
      {{10, 4, 7}, {11, 5, 8}, {12, 6, 9}}};
  std::tuple<int, std::uint16_t, std::size_t> best{-1, std::uint16_t{0},
                                                   std::size_t{0}};
  for (std::size_t category = 0; category < medal_minimums.size(); ++category) {
    for (std::size_t tier = 0; tier < medal_minimums[category].size(); ++tier) {
      if (scores[category] < medal_minimums[category][tier]) {
        continue;
      }
      // Highest tier, then greatest count; equal counts prefer rescue,
      // pang, then bubble. Local tie policy never depends on seat order.
      const auto candidate =
          std::tuple{static_cast<int>(tier), scores[category], category};
      if (candidate > best) {
        best = candidate;
        result.medal = medals[category][tier];
      }
    }
  }
  return result;
}
}  // namespace bubble_fighter::server::features::rooms
