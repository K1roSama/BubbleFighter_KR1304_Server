#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_HOST_CHANGED_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_HOST_CHANGED_NOTIFICATION_H_

#include <cstdint>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R16: S502, room dispatcher 0x009D9930 case 0x1F6 ->
// 0x009DA3C0. Stores the slot as the room host (context +552, the S117
// ValueAt544 holder), clears that slot's ready byte and refreshes the
// room stage buttons.
struct RoomHostChangedNotification {
  static constexpr std::uint16_t kMessageId = 0x01F6;

  std::uint32_t host_slot = 0;

  TcpPacket Encode() const;
  static RoomHostChangedNotification Decode(const TcpPacket& packet);
  bool operator==(const RoomHostChangedNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_HOST_CHANGED_NOTIFICATION_H_
