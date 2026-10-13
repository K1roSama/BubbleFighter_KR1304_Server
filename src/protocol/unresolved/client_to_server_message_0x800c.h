#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X800C_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X800C_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 writer/reader evidence: Docs/Protocol/ActorActions.md.
// Neutral name until the full game action semantics are established.
struct ClientToServerMessage0x800C {
  static constexpr std::uint16_t kMessageId = 0x800C;
  std::uint32_t actor_object_id = 0;
  std::uint32_t value_at4 = 0;
  std::uint8_t axis_at8 = 0;
  std::uint8_t axis_at9 = 0;
  TcpPacket Encode() const;
  static ClientToServerMessage0x800C Decode(const TcpPacket& packet);
  bool operator==(const ClientToServerMessage0x800C&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X800C_H_
