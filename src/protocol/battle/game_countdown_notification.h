#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_COUNTDOWN_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_COUNTDOWN_NOTIFICATION_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E069: 0x009E1EB0 case 607 -> 0x009E7080 sets the pregame timer.
// This is a wire value, not an approved countdown policy.
struct GameCountdownNotification {
  static constexpr std::uint16_t kMessageId = 607;
  std::uint32_t duration_milliseconds = 0;

  TcpPacket Encode() const;
  static GameCountdownNotification Decode(const TcpPacket& packet);
  bool operator==(const GameCountdownNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_COUNTDOWN_NOTIFICATION_H_
