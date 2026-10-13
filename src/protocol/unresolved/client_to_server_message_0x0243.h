#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0243_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0243_H_
#include <array>
#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E079: 0x006DA5D0 -> 0x010D14C0, after map loading. Offsets are
// x86 object offsets, not wire offsets. Resource semantics remain partial.
struct ClientToServerMessage0x0243 {
  static constexpr std::uint16_t kMessageId = 579;
  using Triple = std::array<std::uint32_t, 3>;  // Preserve raw float bits.
  std::uint32_t value_at0 = 0;
  std::vector<Triple> triples_at4;
  std::vector<std::uint32_t> values_at16;
  std::vector<std::uint8_t> bytes_at28;
  std::vector<std::uint32_t> values_at40, values_at52, values_at64;
  std::vector<Triple> triples_at76;
  std::vector<std::uint32_t> values_at88, values_at100, values_at112;
  std::vector<Triple> triples_at124;
  std::vector<std::uint32_t> values_at136, values_at148;
  std::vector<std::u16string> strings_at160;

  TcpPacket Encode() const;
  static ClientToServerMessage0x0243 Decode(const TcpPacket& packet);
  bool operator==(const ClientToServerMessage0x0243&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_UNRESOLVED_CLIENT_TO_SERVER_MESSAGE_0X0243_H_
