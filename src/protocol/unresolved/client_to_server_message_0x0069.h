#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0069_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0069_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 0x0063A510: empty C105. Observed statically in the stage-3
// task-bar path; keep direction/number until all callers are understood.
struct ClientToServerMessage0x0069 {
  static constexpr std::uint16_t kMessageId = 0x0069;
  TcpPacket Encode() const;
  static ClientToServerMessage0x0069 Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0069_H_
