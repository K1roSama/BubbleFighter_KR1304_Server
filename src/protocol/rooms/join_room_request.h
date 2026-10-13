#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_JOIN_ROOM_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_JOIN_ROOM_REQUEST_H_

#include <cstdint>
#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R17: C403, written by the room-list join handler at
// 0x00855454 (function 0x00854C90).
struct JoinRoomRequest {
  static constexpr std::uint16_t kMessageId = 0x0193;

  // The listed descriptor's ShortAt0, sign-extended to 32 bits.
  std::uint32_t room_id = 0;
  // Empty unless the listed room has BooleanAt38 set; the client then
  // asks for it in a dialog before sending.
  std::u16string password;

  TcpPacket Encode() const;
  static JoinRoomRequest Decode(const TcpPacket& packet);
  bool operator==(const JoinRoomRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_JOIN_ROOM_REQUEST_H_
