#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_STOMP_HIT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_STOMP_HIT_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 8E1E10 state 117 -> 6F7780 -> 4A8D30/4A91F0.
struct GameStompHitNotification {
  static constexpr std::uint16_t kMessageId = 0x8030;
  std::uint32_t actor_object_id = 0;
  std::uint32_t target_object_id = 0;
  TcpPacket Encode() const;
  static GameStompHitNotification Decode(const TcpPacket& packet);
  bool operator==(const GameStompHitNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_STOMP_HIT_NOTIFICATION_H_
