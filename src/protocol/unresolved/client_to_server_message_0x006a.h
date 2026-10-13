#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X006A_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X006A_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E042/E043: empty packet sent by the plaza button after action gate 2.
// Other callers exist; complete original server semantics remain unknown.
struct ClientToServerMessage0x006A {
  static constexpr std::uint16_t kMessageId = 0x006A;
  TcpPacket Encode() const;
  static ClientToServerMessage0x006A Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X006A_H_
