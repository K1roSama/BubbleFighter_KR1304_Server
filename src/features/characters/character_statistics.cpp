#include "features/characters/character_statistics.h"

namespace bubble_fighter::server::features::characters {
bool CharacterStatistics::IsValid() const {
  // Local storage bounds keep values nonnegative in signed client logic.
  // This is not evidence of the game's maximum level or wallet balance.
  return level >= 1 && level <= kMaxValue && experience <= kMaxValue &&
         lucci <= kMaxValue && cash <= kMaxValue;
}
}  // namespace bubble_fighter::server::features::characters
