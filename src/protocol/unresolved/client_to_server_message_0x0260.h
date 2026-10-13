#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0260_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0260_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E085: 0x006DDBA0. Offsets below refer to the packed wire payload.
// Event-specific meanings/authority are not inferred from this layout.
struct ClientToServerMessage0x0260 {
  static constexpr std::uint16_t kMessageId = 608;
  std::uint8_t event_code = 0;
  std::uint32_t value_at1 = 0;
  std::uint32_t value_at5 = 0;
  std::uint8_t byte_at9 = 0;
  std::uint8_t byte_at10 = 0;
  std::uint8_t byte_at11 = 0;
  std::uint16_t short_at12 = 0;
  std::uint16_t short_at14 = 0;
  TcpPacket Encode() const;
  static ClientToServerMessage0x0260 Decode(const TcpPacket& packet);
  bool operator==(const ClientToServerMessage0x0260&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0260_H_
