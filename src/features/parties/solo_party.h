#ifndef BUBBLEFIGHTER_SERVER_FEATURES_PARTIES_SOLO_PARTY_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_PARTIES_SOLO_PARTY_H_
#include "features/characters/character_store.h"
#include "protocol/scenes/update_party_notification.h"

namespace bubble_fighter::server::features::parties {
class SoloParty {
 public:
  static protocol::UpdatePartyNotification Create(
      const characters::CharacterRecord& character);
};
}  // namespace bubble_fighter::server::features::parties

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_PARTIES_SOLO_PARTY_H_
