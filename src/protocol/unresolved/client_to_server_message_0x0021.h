#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0021_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0021_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E028: empty follow-up sent from the create response handler.
// The send site alone does not establish the server-side operation.
struct ClientToServerMessage0x0021 {
  static constexpr std::uint16_t kMessageId = 0x0021;

  TcpPacket Encode() const;
  static ClientToServerMessage0x0021 Decode(const TcpPacket& packet);
  bool operator==(const ClientToServerMessage0x0021&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0021_H_
