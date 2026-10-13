#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_START_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_START_NOTIFICATION_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E069: 0x009E1EB0 case 608 -> 0x009E7570, GameStart event and timer.
// E071: engine+2152 -> 0x006D1EA0 -> 0x00F99280 RNG initialization.
// Also echoed by C103; it is not a unique match ID or an authorization token.
struct GameStartNotification {
  static constexpr std::uint16_t kMessageId = 608;
  std::uint32_t duration_milliseconds = 0;
  std::uint32_t random_seed = 0;

  TcpPacket Encode() const;
  static GameStartNotification Decode(const TcpPacket& packet);
  bool operator==(const GameStartNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_START_NOTIFICATION_H_
