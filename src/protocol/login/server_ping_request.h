#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_SERVER_PING_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_SERVER_PING_REQUEST_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E076: S4 dispatches gspReqPing. The verified prefix needs no payload;
// the minimal empty request is local policy, with its protected tail
// unverified.
struct ServerPingRequest {
  static constexpr std::uint16_t kMessageId = 4;
  TcpPacket Encode() const;
  static ServerPingRequest Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_SERVER_PING_REQUEST_H_
