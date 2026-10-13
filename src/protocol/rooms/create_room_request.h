#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_CREATE_ROOM_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_CREATE_ROOM_REQUEST_H_

#include <cstdint>
#include <optional>
#include <string>

#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/scenes/session_descriptor.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R1: BFCLIENT02 0x00962200 constructs the room settings with
// 0x0113B7B0, writes message 0x0192 and serializes them with 0x0113BAB0
// (the SessionDescriptor writer), then the password, a 0x3A/0xBA marker
// and, only when present, a uint32. Field meanings:
// Docs/Protocol/RoomCreation.md.
struct CreateRoomRequest {
  static constexpr std::uint16_t kMessageId = 0x0192;
  static constexpr std::uint8_t kPresentMarker = 0x3A;
  static constexpr std::uint8_t kAbsentMarker = 0xBA;

  SessionDescriptor settings;
  // 0x00962272: BooleanAt38 is derived from this string being non-empty.
  std::u16string password;
  // Lobby dialog 0x00855AE0 sends it absent; 0x00BBCCE0 sends present 0.
  // Meaning unknown; the marker is canonical because the writer derives it.
  std::optional<std::uint32_t> trailing_value;

  TcpPacket Encode() const;
  static CreateRoomRequest Decode(const TcpPacket& packet);
  bool operator==(const CreateRoomRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_CREATE_ROOM_REQUEST_H_
