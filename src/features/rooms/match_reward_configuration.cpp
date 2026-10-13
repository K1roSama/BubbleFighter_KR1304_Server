#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <istream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>

#include "features/characters/character_statistics.h"
#include "features/rooms/match_reward_policy.h"

namespace bubble_fighter::server::features::rooms {
namespace {
constexpr auto kMax = characters::CharacterStatistics::kMaxValue;
}

MatchRewardPolicy MatchRewardPolicy::Read(std::istream& input) {
  MatchRewardPolicy rules;
  std::map<std::string, std::uint32_t*> fields{
      {"ExperiencePerLevel", &rules.experience_per_level},
      {"MaximumLevel", &rules.maximum_level},
      {"CompletionExperience", &rules.completion_experience},
      {"CompletionLucci", &rules.completion_lucci},
      {"WinExperience", &rules.win_experience},
      {"WinLucci", &rules.win_lucci},
      {"PangExperience", &rules.pang_experience},
      {"PangLucci", &rules.pang_lucci},
      {"RescueExperience", &rules.rescue_experience},
      {"RescueLucci", &rules.rescue_lucci}};
  constexpr std::array grades{"SSS", "SS", "S", "A", "B", "C", "D", "E"};
  constexpr std::array categories{"Bubble", "Pang", "Rescue"};
  constexpr std::array tiers{"Beginner", "Expert", "Master"};
  for (std::size_t i = 0; i < grades.size(); ++i) {
    fields.emplace(std::string("Grade") + grades[i] + "Minimum",
                   &rules.awards.grade_minimums[i]);
  }
  for (std::size_t i = 0; i < categories.size(); ++i) {
    for (std::size_t j = 0; j < tiers.size(); ++j) {
      fields.emplace(
          std::string("Medal") + categories[i] + tiers[j] + "Minimum",
          &rules.awards.medal_minimums[i][j]);
    }
  }
  std::set<std::string> seen;
  std::string line;
  std::size_t bytes = 0;
  while (std::getline(input, line)) {
    bytes += line.size() + 1;
    if (bytes > 16384) {
      throw std::invalid_argument("Local match rules file is too large.");
    }
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty() || line.front() == '#') {
      continue;
    }
    const auto equals = line.find('=');
    const auto name = line.substr(0, equals);
    const auto field = fields.find(name);
    if (equals == std::string::npos || field == fields.end() ||
        !seen.insert(name).second) {
      throw std::invalid_argument("Unknown or duplicate local match rule.");
    }
    const auto value = std::string_view(line).substr(equals + 1);
    const auto parsed = std::from_chars(
        value.data(), value.data() + value.size(), *field->second);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) {
      throw std::invalid_argument(
          "Local match rules require unsigned decimal values.");
    }
  }
  if (input.bad()) {
    throw std::runtime_error("Cannot read local match rules.");
  }
  rules.Validate();
  return rules;
}

MatchRewardPolicy MatchRewardPolicy::Load(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Cannot open local match rules.");
  }
  return Read(file);
}
}  // namespace bubble_fighter::server::features::rooms
