#include "features/characters/character_profile.h"

#include <limits>
#include <stdexcept>

#include "features/characters/character_creation.h"

namespace bubble_fighter::server::features::characters {
protocol::UserStatNotification CreateCharacterProfileBaseline() {
  protocol::UserStatNotification profile;
  // 0x01159720 / 0x011527E0 native defaults; not original-server rewards.
  profile.fighter.values_at296 = {1000, 1};
  for (std::uint16_t key = 0; key != 6; ++key) {
    profile.fighter.abilities.push_back({key, 0});
  }
  return profile;
}

protocol::UserStatNotification CharacterProfile::BindKnownFields(
    protocol::UserStatNotification envelope, const CharacterRecord& character,
    std::uint64_t authenticated_account_id) {
  if (authenticated_account_id == 0 ||
      character.account_id != authenticated_account_id ||
      character.character_id == 0 ||
      character.character_id > std::numeric_limits<std::uint32_t>::max() ||
      CharacterCreation::ValidateName(character.character_name) != 0 ||
      !CharacterCreation::SupportsSelection(character.character_type,
                                            character.color_index) ||
      !character.statistics.IsValid()) {
    throw std::invalid_argument(
        "Invalid persisted character profile or account ownership.");
  }

  // 0x009A5C40 -> local player +56/+68/+76/+80. UI color and named
  // lblLucci/lblCash consumers confirm their meanings (E051).
  envelope.values[0] = character.color_index;
  envelope.values[1] = character.statistics.experience;
  envelope.values[4] = character.statistics.lucci;
  envelope.values[5] = character.statistics.cash;
  // FighterInfo +36 becomes local player +60; S107 and LevelSymbolIcon
  // independently confirm level. +40 is not a third currency balance.
  envelope.fighter.values_at32[1] = character.statistics.level;
  return envelope;
}
}  // namespace bubble_fighter::server::features::characters
