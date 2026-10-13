#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_MAP_CATALOG_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_MAP_CATALOG_H_
#include <span>
#include <string_view>

#include "protocol/rooms/start_game_request.h"

namespace bubble_fighter::server::features::rooms {
struct RoomMapDefinition {
  std::u16string_view key, directory, name;
  std::uint32_t difficulty;
  protocol::StartGameRules rules;
};
class RoomMapCatalog {
 public:
  static std::span<const RoomMapDefinition> Maps();
  static const RoomMapDefinition* Match(
      const protocol::StartGameRequest& request);
};
}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_MAP_CATALOG_H_
