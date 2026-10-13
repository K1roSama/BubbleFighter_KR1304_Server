#ifndef BUBBLEFIGHTER_SERVER_SESSIONS_MENU_SNAPSHOT_H_
#define BUBBLEFIGHTER_SERVER_SESSIONS_MENU_SNAPSHOT_H_

#include "sessions/menu_initialization.h"

namespace bubble_fighter::server::sessions {
// Local solo-menu policy using BFCLIENT02 native constructor defaults.
// Inventory must have been loaded successfully for this owned character.
MenuInitializationData CreateMenuSnapshot(
    const features::characters::CharacterRecord& character,
    features::inventory::CharacterInventory inventory,
    std::uint64_t observed_at_unix_seconds);
}  // namespace bubble_fighter::server::sessions

#endif  // BUBBLEFIGHTER_SERVER_SESSIONS_MENU_SNAPSHOT_H_
