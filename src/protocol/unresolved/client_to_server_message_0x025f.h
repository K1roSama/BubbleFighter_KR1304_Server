#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X025F_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X025F_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E079: 0x0075544A takes the local actor's +448 field; 0x006DDE20
// writes it as one uint32. It is not an account/character database ID.
struct ClientToServerMessage0x025F {
  static constexpr std::uint16_t kMessageId = 607;
  std::uint32_t value_at0 = 0;
  TcpPacket Encode() const;
  static ClientToServerMessage0x025F Decode(const TcpPacket& packet);
  bool operator==(const ClientToServerMessage0x025F&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X025F_H_
