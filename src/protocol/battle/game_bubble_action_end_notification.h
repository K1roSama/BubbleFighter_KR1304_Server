#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_ACTION_END_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_ACTION_END_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 8F0EF0 writer; 6F7600 requests actor state 116.
struct GameBubbleActionEndNotification {
  static constexpr std::uint16_t kMessageId = 0x8018;
  std::uint32_t actor_object_id = 0;
  TcpPacket Encode() const;
  static GameBubbleActionEndNotification Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleActionEndNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_ACTION_END_NOTIFICATION_H_
