#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0270_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0270_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E071: 0x00708B60 writes uint32 + zero-extended uint8 as uint32.
// Training btnQuit calls 0x006E5840(1), emitting {1, 0}. Other uses unknown.
struct ClientToServerMessage0x0270 {
  static constexpr std::uint16_t kMessageId = 624;
  std::uint32_t value_at0 = 0;
  std::uint32_t value_at4 = 0;
  TcpPacket Encode() const;
  static ClientToServerMessage0x0270 Decode(const TcpPacket& packet);
  bool operator==(const ClientToServerMessage0x0270&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0270_H_
