#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_ACTOR_DEATH_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_ACTOR_DEATH_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E082: 0x006E2CA0 writes the actor ID; 0x006D8743 reads it and
// applies Character state 402. Shared wire form in both directions.
struct GameActorDeathNotification {
  static constexpr std::uint16_t kMessageId = 0x9582;
  std::uint32_t actor_object_id = 0;

  TcpPacket Encode() const;
  static GameActorDeathNotification Decode(const TcpPacket& packet);
  bool operator==(const GameActorDeathNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_ACTOR_DEATH_NOTIFICATION_H_
