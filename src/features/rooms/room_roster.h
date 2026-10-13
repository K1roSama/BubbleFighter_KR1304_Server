#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_ROSTER_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_ROSTER_H_

#include <cstdint>

#include "features/rooms/room_registry.h"
#include "protocol/rooms/room_member_left_notification.h"
#include "protocol/rooms/room_slot_update_notification.h"
#include "protocol/scenes/goto_room_notification.h"

namespace bubble_fighter::server::features::rooms {
// Packets derived from a room record. Pure: nothing here sends or stores.
class RoomRoster {
 public:
  // The descriptor of a room, shared by its S117 and its S101 entry:
  // server identity, type 2, state and the counts the list renders as
  // "(ShortsAt32[0] - ShortsAt32[1]) / ShortsAt32[2]" (0x0063CF90).
  static protocol::SessionDescriptor Describe(const RoomRecord& room);
  // Room S117 for the member in localSlot (0x009D9250 bounds the slots).
  static protocol::GotoRoomNotification Snapshot(const RoomRecord& room,
                                                 std::uint32_t local_slot);
  // S503 for an occupied slot: the member and its two equipment records
  // exactly as the room S117 carries them.
  static protocol::RoomSlotUpdateNotification SlotUpdate(const RoomRecord& room,
                                                         std::uint32_t slot);
  // S511 emptying a slot.
  static protocol::RoomMemberLeftNotification MemberLeft(const RoomRecord& room,
                                                         std::uint32_t slot);
};
}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_ROSTER_H_
