#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_ACTION_COMPLETE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_ACTION_COMPLETE_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 8F2500 -> 6E2EC0; 6D55D0 requests state 115 and 4AE3F0.
// Distinct from 8018, which requests state 116.
struct GameBubbleActionCompleteNotification {
  static constexpr std::uint16_t kMessageId = 0x8045;
  std::uint32_t actor_object_id = 0;
  TcpPacket Encode() const;
  static GameBubbleActionCompleteNotification Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleActionCompleteNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_ACTION_COMPLETE_NOTIFICATION_H_
