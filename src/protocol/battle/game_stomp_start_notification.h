#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_STOMP_START_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_STOMP_START_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 492E70 -> 6E4730; 6D55D0 -> 4A8BA0 (state 117).
struct GameStompStartNotification {
  static constexpr std::uint16_t kMessageId = 0x802F;
  std::uint32_t actor_object_id = 0;
  std::uint32_t target_object_id = 0;
  TcpPacket Encode() const;
  static GameStompStartNotification Decode(const TcpPacket& packet);
  bool operator==(const GameStompStartNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_STOMP_START_NOTIFICATION_H_
