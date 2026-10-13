#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_READY_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_READY_REQUEST_H_

#include <cstdint>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R19: C505, written by the room stage update 0x009785E0 when
// the local room state changes. State 0 -> {0,0,0}, 1 (ready button
// +0x644) -> {1,0,0}, 3 -> {0,1,0}, 2 -> {0,0,1}; states 2 and 3 come
// from client UI flags (0x018E566C +260 / +256) whose meaning is unknown.
// Values are zero-extended bytes.
struct RoomReadyRequest {
  static constexpr std::uint16_t kMessageId = 0x01F9;

  std::uint32_t ready = 0;
  std::uint32_t value_at4 = 0;
  std::uint32_t value_at8 = 0;

  TcpPacket Encode() const;
  static RoomReadyRequest Decode(const TcpPacket& packet);
  bool operator==(const RoomReadyRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_READY_REQUEST_H_
