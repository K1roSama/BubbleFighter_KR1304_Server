#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_ACTOR_RESPAWN_WAIT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_ACTOR_RESPAWN_WAIT_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E083: 0x006E2DB0 writes 0x8044; 0x006F6900 invokes the actor's
// +572 callback, which selects state 401 and starts its respawn wait.
struct GameActorRespawnWaitNotification {
  static constexpr std::uint16_t kMessageId = 0x8044;
  std::uint32_t actor_object_id = 0;
  TcpPacket Encode() const;
  static GameActorRespawnWaitNotification Decode(const TcpPacket& packet);
  bool operator==(const GameActorRespawnWaitNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_ACTOR_RESPAWN_WAIT_NOTIFICATION_H_
