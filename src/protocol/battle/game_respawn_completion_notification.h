#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESPAWN_COMPLETION_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESPAWN_COMPLETION_NOTIFICATION_H_
#include "protocol/battle/respawn_parameters.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E080: client-generated 0x809E passed to routing helper 0x007AB190,
// also consumed by the game dispatcher. Its actual route is state-dependent.
struct GameRespawnCompletionNotification {
  static constexpr std::uint16_t kMessageId = 0x809E;
  RespawnParameters parameters;
  std::uint8_t byte_at20 = 0;
  std::uint32_t value_at21 = 0;
  TcpPacket Encode() const;
  static GameRespawnCompletionNotification Decode(const TcpPacket& packet);
  bool operator==(const GameRespawnCompletionNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESPAWN_COMPLETION_NOTIFICATION_H_
