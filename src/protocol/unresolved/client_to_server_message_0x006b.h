#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X006B_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X006B_H_

#include <array>
#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E068: start request emitted by 0x0099F580. Selector zero now chooses
// a training map, not the older client's prologue. No mode is inferred.
struct ClientToServerMessage0x006B {
  static constexpr std::uint16_t kMessageId = 107;
  static constexpr std::size_t kMaxPayloadBytes = 65536;
  // Client object offsets 0..48. Resource names/units are in TutorialEntry.md.
  std::array<std::uint32_t, 13> rules{};
  std::uint8_t rule_flags = 0;  // Client offset 52; exactly ONE byte on wire.
  std::u16string directory;
  std::u16string map_name;
  std::u16string map_key;
  std::uint32_t difficulty = 0;

  TcpPacket Encode() const;
  static ClientToServerMessage0x006B Decode(const TcpPacket& packet);
  bool operator==(const ClientToServerMessage0x006B&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X006B_H_
