#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CENTER_LIST_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CENTER_LIST_NOTIFICATION_H_

#include <array>
#include <cstdint>
#include <vector>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct CenterGateInfo {
  std::uint32_t unknown_value_at4 = 0;
  std::uint32_t unknown_value_at8 = 0;
  std::uint32_t unknown_value_at12 = 0;
  std::array<std::uint8_t, 4> ipv4_address{};
  std::uint16_t port = 0;
  bool operator==(const CenterGateInfo&) const = default;
};

struct CenterListNotification {
  static constexpr std::uint16_t kMessageId = 0x0011;
  // Local allocation limit, not a claim about the original server.
  static constexpr std::uint32_t kMaxEntries = 1024;
  std::vector<CenterGateInfo> gates;
  TcpPacket Encode() const;
  static CenterListNotification Decode(const TcpPacket& packet);
  bool operator==(const CenterListNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CENTER_LIST_NOTIFICATION_H_
