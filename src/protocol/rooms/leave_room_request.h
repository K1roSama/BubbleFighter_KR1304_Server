#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_LEAVE_ROOM_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_LEAVE_ROOM_REQUEST_H_

#include <cstdint>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R18: C404, built empty by 0x0062A880. The room task bar
// (0x00AA5B50) sends it and changes no stage itself: the server must
// move the client out of the room.
struct LeaveRoomRequest {
  static constexpr std::uint16_t kMessageId = 0x0194;

  TcpPacket Encode() const;
  static LeaveRoomRequest Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_LEAVE_ROOM_REQUEST_H_
