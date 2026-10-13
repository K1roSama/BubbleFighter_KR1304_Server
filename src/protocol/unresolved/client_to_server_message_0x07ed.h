#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X07ED_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X07ED_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E085: 0x00708660. Field offsets are wire offsets, not native padding.
// The 0x006DDBA0 switch is not a global EventCode whitelist.
struct ClientToServerMessage0x07ED {
  static constexpr std::uint16_t kMessageId = 2029;
  std::uint8_t event_code = 0;
  std::uint32_t value_at1 = 0;
  std::uint8_t byte_at5 = 0;
  TcpPacket Encode() const;
  static ClientToServerMessage0x07ED Decode(const TcpPacket& packet);
  bool operator==(const ClientToServerMessage0x07ED&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X07ED_H_
