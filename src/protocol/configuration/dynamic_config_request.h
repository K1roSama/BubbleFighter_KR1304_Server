#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CONFIGURATION_DYNAMIC_CONFIG_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CONFIGURATION_DYNAMIC_CONFIG_REQUEST_H_

#include <array>
#include <optional>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct DynamicConfigRequest {
  static constexpr std::uint16_t kMessageId = 0x05DC;
  std::array<std::optional<std::uint32_t>, 32> checksums;
  TcpPacket Encode() const;
  static DynamicConfigRequest Decode(const TcpPacket& packet);
  bool operator==(const DynamicConfigRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CONFIGURATION_DYNAMIC_CONFIG_REQUEST_H_
