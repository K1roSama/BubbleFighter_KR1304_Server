#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_RESCUE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_RESCUE_NOTIFICATION_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomBubble: 85CCCB / 6D97D0; offset 8 preserves the native float bit pattern.
struct GameBubbleRescueNotification {
  static constexpr std::uint16_t kMessageId = 0x9581;
  std::uint32_t actor_object_id = 0;
  std::uint32_t helper_object_id = 0;
  std::uint32_t value_at8 = 0;
  std::uint8_t kind = 0;
  TcpPacket Encode() const;
  static GameBubbleRescueNotification Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleRescueNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_RESCUE_NOTIFICATION_H_
