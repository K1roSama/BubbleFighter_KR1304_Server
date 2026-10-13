#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_MOVE_INTO_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_MOVE_INTO_RESPONSE_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E024: gspRepMoveInto, read by BFCLIENT02 at 0x009A6D30.
struct MoveIntoResponse {
  static constexpr std::uint16_t kMessageId = 0x000B;

  // The client converts every nonzero value to true. Keep all wire bits.
  std::uint32_t accepted_value = 0;
  std::uint32_t channel_category = 0;
  std::uint32_t unknown_connection_value = 0;

  bool IsAccepted() const noexcept;
  TcpPacket Encode() const;
  static MoveIntoResponse Decode(const TcpPacket& packet);
  bool operator==(const MoveIntoResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_MOVE_INTO_RESPONSE_H_
