#ifndef BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_PROFILE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_PROFILE_H_

#include "features/characters/character_store.h"
#include "protocol/characters/user_stat_notification.h"

namespace bubble_fighter::server::features::characters {
// Recovered native FighterInfo baseline shared by menu and settlement.
// This carries no rewards, inventory grants or scene initialization.
protocol::UserStatNotification CreateCharacterProfileBaseline();

class CharacterProfile {
 public:
  // Binds only E051-confirmed fields. The caller must supply all remaining
  // fields from evidence-backed state; this does not create/send S105.
  static protocol::UserStatNotification BindKnownFields(
      protocol::UserStatNotification envelope, const CharacterRecord& character,
      std::uint64_t authenticated_account_id);
};
}  // namespace bubble_fighter::server::features::characters

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_PROFILE_H_
