#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_REPORT_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_REPORT_REQUEST_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E071: 0x009E80F0 -> 0x0113F230, 30 uint32 fields (120 payload bytes).
// Reported counters are untrusted and their individual meanings unresolved.
struct GameReportRequest {
  static constexpr std::uint16_t kMessageId = 103;
  std::uint32_t random_seed = 0;
  std::array<std::uint32_t, 29> values_at4{};
  TcpPacket Encode() const;
  static GameReportRequest Decode(const TcpPacket& packet);
  bool operator==(const GameReportRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_REPORT_REQUEST_H_
