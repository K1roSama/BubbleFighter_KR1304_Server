#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_SERVER_TO_CLIENT_MESSAGE_0X025E_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_SERVER_TO_CLIENT_MESSAGE_0X025E_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E069: 0x006D55D0 case 606 reads a uint32 without using its value,
// sets GameStage +2185 and calls 0x009E4F30 to check scene consistency.
// Do not interpret ValueAt0 as a result code or local slot.
struct ServerToClientMessage0x025E {
  static constexpr std::uint16_t kMessageId = 606;
  std::uint32_t value_at0 = 0;

  TcpPacket Encode() const;
  static ServerToClientMessage0x025E Decode(const TcpPacket& packet);
  bool operator==(const ServerToClientMessage0x025E&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_SERVER_TO_CLIENT_MESSAGE_0X025E_H_
