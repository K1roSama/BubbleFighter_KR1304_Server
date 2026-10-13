#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_DAMAGER_REMOVE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_DAMAGER_REMOVE_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct GameDamagerRemoveNotification {
  static constexpr std::uint16_t kMessageId = 0x808B;
  std::uint32_t damager_key = 0;
  TcpPacket Encode() const;
  static GameDamagerRemoveNotification Decode(const TcpPacket& packet);
  bool operator==(const GameDamagerRemoveNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_DAMAGER_REMOVE_NOTIFICATION_H_
