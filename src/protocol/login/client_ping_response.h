#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CLIENT_PING_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CLIENT_PING_RESPONSE_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E076: original protected S4 handler constructs C41 with no payload.
struct ClientPingResponse {
  static constexpr std::uint16_t kMessageId = 41;
  TcpPacket Encode() const;
  static ClientPingResponse Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CLIENT_PING_RESPONSE_H_
