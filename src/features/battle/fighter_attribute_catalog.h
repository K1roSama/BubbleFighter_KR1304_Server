#ifndef BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_FIGHTER_ATTRIBUTE_CATALOG_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_FIGHTER_ATTRIBUTE_CATALOG_H_
#include <cstdint>
#include <optional>

namespace bubble_fighter::server::features::battle {
enum class FighterAttributeVariant { kNormal, kSeason, kBubbleCup };
struct FighterBaseAttributes {
  float hp, bubble_hp;
  bool operator==(const FighterBaseAttributes&) const = default;
};
class FighterAttributeCatalog {
 public:
  // Caller supplies native TABLE keys, not arbitrary gameplay mode IDs.
  // No fallback for unknown keys; no ownership grant. Result precedes
  // equipment, skill and runtime modifiers and is not final maximum HP.
  static std::optional<FighterBaseAttributes> Resolve(
      std::uint32_t character, std::uint32_t channel_table_key,
      std::uint32_t mode_table_key, FighterAttributeVariant variant);
};
}  // namespace bubble_fighter::server::features::battle

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_BATTLE_FIGHTER_ATTRIBUTE_CATALOG_H_
