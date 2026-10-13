#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESPAWN_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESPAWN_NOTIFICATION_H_
#include "protocol/battle/respawn_parameters.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// Main TCP S632, consumed by 0x006F5E50. ValueAt24 is written to
// the scene member's +304 field; its business meaning is not confirmed.
struct GameRespawnNotification {
  static constexpr std::uint16_t kMessageId = 632;
  RespawnParameters parameters;
  std::uint32_t reset_items_value = 0;
  std::uint32_t value_at24 = 0;
  TcpPacket Encode() const;
  static GameRespawnNotification Decode(const TcpPacket& packet);
  bool operator==(const GameRespawnNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESPAWN_NOTIFICATION_H_
