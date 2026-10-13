#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESPAWN_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESPAWN_REQUEST_H_
#include "protocol/battle/respawn_parameters.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// Main TCP C602. itemResetOnRespawn[Single] supplies the trailing
// uint32 at the sender; no server authorization is implied by decoding.
struct GameRespawnRequest {
  static constexpr std::uint16_t kMessageId = 602;
  RespawnParameters parameters;
  std::uint32_t reset_items_value = 0;
  TcpPacket Encode() const;
  static GameRespawnRequest Decode(const TcpPacket& packet);
  bool operator==(const GameRespawnRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESPAWN_REQUEST_H_
