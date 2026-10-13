#include "features/battle/fighter_attribute_catalog.h"

#include <algorithm>
#include <array>
#include <stdexcept>

#include "features/battle/generated/fighter_attribute_catalog_data.h"

namespace bubble_fighter::server::features::battle {
std::optional<FighterBaseAttributes> FighterAttributeCatalog::Resolve(
    std::uint32_t character, std::uint32_t channel_table_key,
    std::uint32_t mode_table_key, FighterAttributeVariant variant) {
  if (variant != FighterAttributeVariant::kNormal &&
      variant != FighterAttributeVariant::kSeason &&
      variant != FighterAttributeVariant::kBubbleCup) {
    throw std::invalid_argument("Unknown fighter attribute variant.");
  }
  const std::array tables{0U, 1U, 2U, 3U + static_cast<unsigned>(variant)};
  const std::array keys{0U, channel_table_key, mode_table_key, character};
  FighterBaseAttributes result{};
  for (std::size_t i = 0; i < tables.size(); ++i) {
    const auto found =
        std::find_if(std::begin(kFighterAttributeRows),
                     std::end(kFighterAttributeRows), [&](const auto& row) {
                       return row.table == tables[i] && row.key == keys[i];
                     });
    if (found == std::end(kFighterAttributeRows)) {
      return std::nullopt;
    }
    result.hp += found->hp;
    result.bubble_hp = std::max(0.0f, result.bubble_hp + found->bubble_hp);
  }
  return result;
}
}  // namespace bubble_fighter::server::features::battle
