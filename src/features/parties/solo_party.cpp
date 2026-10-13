#include "features/parties/solo_party.h"

#include <limits>
#include <stdexcept>

#include "features/characters/character_creation.h"

namespace bubble_fighter::server::features::parties {
protocol::UpdatePartyNotification SoloParty::Create(
    const characters::CharacterRecord& character) {
  if (character.account_id == 0 || character.character_id == 0 ||
      character.character_id > std::numeric_limits<std::uint32_t>::max() ||
      characters::CharacterCreation::ValidateName(character.character_name) !=
          0) {
    throw std::invalid_argument("Invalid persisted solo party member.");
  }
  protocol::UpdatePartyNotification party;
  // E041: state 2 permits the ordinary move path, and member zero is leader.
  // Local policy: a fresh authenticated connection starts as a solo party.
  party.state = 2;
  party.members.push_back({static_cast<std::uint32_t>(character.character_id),
                           character.character_name,
                           {0xFFFFFFFFU, 0xFFFFFFFFU, 0, 0}});
  // E041: the four opaque fields retain the original PartyMember defaults.
  // No supplementary records are issued until their gameplay meaning is known.
  return party;
}
}  // namespace bubble_fighter::server::features::parties
