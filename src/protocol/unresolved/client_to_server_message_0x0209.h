#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0209_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0209_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E054: empty packet from 0x00937560 / 0x00937920 during common UI
// initialization. Its original request/response meaning remains unknown.
struct ClientToServerMessage0x0209 {
  static constexpr std::uint16_t kMessageId = 0x0209;
  TcpPacket Encode() const;
  static ClientToServerMessage0x0209 Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0209_H_
