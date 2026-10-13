#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESULT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESULT_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E072: 0x009E308C reads 4+4+1+1 bytes; 0x009E9710 enters Result.
// Header/counter meaning remains unknown. Codec only, not sent by practice.
struct GameResultNotification {
  static constexpr std::uint16_t kMessageId = 613;
  std::uint32_t value_at0 = 0;
  std::uint32_t elapsed_milliseconds =
      0;  // 0x00D9FDB0 divides by 1000 for Match_RoundEnd.
  std::uint8_t byte_at8 = 0;
  std::uint8_t byte_at9 = 0;
  TcpPacket Encode() const;
  static GameResultNotification Decode(const TcpPacket& packet);
  bool operator==(const GameResultNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESULT_NOTIFICATION_H_
