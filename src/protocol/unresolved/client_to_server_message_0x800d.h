#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X800D_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X800D_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 writer/reader evidence: Docs/Protocol/ActorActions.md.
// Neutral name until the full game action semantics are established.
struct ClientToServerMessage0x800D {
  static constexpr std::uint16_t kMessageId = 0x800D;
  std::uint32_t actor_object_id = 0;
  std::uint32_t value_bits_at4 = 0;
  std::uint32_t value_at8 = 0;
  TcpPacket Encode() const;
  static ClientToServerMessage0x800D Decode(const TcpPacket& packet);
  bool operator==(const ClientToServerMessage0x800D&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X800D_H_
