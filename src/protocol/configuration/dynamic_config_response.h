#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CONFIGURATION_DYNAMIC_CONFIG_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CONFIGURATION_DYNAMIC_CONFIG_RESPONSE_H_

#include <array>
#include <optional>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct DynamicConfigResponse {
  static constexpr std::uint16_t kMessageId = 0x05DC;
  // Local aggregate bound, below the TCP payload limit including framing.
  static constexpr std::size_t kMaxCompressedBytes = 512 * 1024;
  std::array<std::optional<std::vector<std::uint8_t>>, 32> records;
  TcpPacket Encode() const;
  static DynamicConfigResponse Decode(const TcpPacket& packet);
  bool operator==(const DynamicConfigResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CONFIGURATION_DYNAMIC_CONFIG_RESPONSE_H_
