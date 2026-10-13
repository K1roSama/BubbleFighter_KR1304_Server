#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_OVER_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_OVER_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E072: 0x009E2FE0 / 0x009E88C0, fixed two uint32 fields.
// Reason 2 selects time-over.png. Other reason values remain unnamed.
struct GameOverNotification {
  static constexpr std::uint16_t kMessageId = 612;
  static constexpr std::uint32_t kTimeOver = 2;
  std::uint32_t reason = 0;
  std::uint32_t value_at4 = 0;
  TcpPacket Encode() const;
  static GameOverNotification Decode(const TcpPacket& packet);
  bool operator==(const GameOverNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_OVER_NOTIFICATION_H_
