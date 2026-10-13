#include "features/rooms/room_map_catalog.h"

#include <algorithm>

#include "features/rooms/generated/room_map_catalog_data.h"

namespace bubble_fighter::server::features::rooms {
std::span<const RoomMapDefinition> RoomMapCatalog::Maps() {
  return kRoomMapDefinitions;
}
const RoomMapDefinition* RoomMapCatalog::Match(
    const protocol::StartGameRequest& request) {
  if (request.variant != 0 || request.flags != 0) {
    return nullptr;
  }
  const auto maps = Maps();
  const auto found =
      std::find_if(maps.begin(), maps.end(), [&](const auto& map) {
        return request.map_resource == map.directory &&
               request.map_attribute == map.name &&
               (request.map_key == map.key || request.map_key == u"Random") &&
               request.value_at76 == map.difficulty &&
               request.rules == map.rules;
      });
  return found == maps.end() ? nullptr : &*found;
}
}  // namespace bubble_fighter::server::features::rooms
