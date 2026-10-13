#ifndef BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_STATISTICS_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_STATISTICS_H_

#include <cstdint>

namespace bubble_fighter::server::features::characters {
// E051 confirms these field meanings, not the original server's rewards or
// level curve. New local accounts start at level 1 with no XP or currency.
struct CharacterStatistics {
  static constexpr std::uint32_t kMaxValue = 0x7FFFFFFFU;

  std::uint32_t level = 1;
  std::uint32_t experience = 0;
  std::uint32_t lucci = 0;
  std::uint32_t cash = 0;

  bool IsValid() const;
  bool operator==(const CharacterStatistics&) const = default;
};
}  // namespace bubble_fighter::server::features::characters

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_STATISTICS_H_
