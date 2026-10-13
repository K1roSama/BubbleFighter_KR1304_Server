#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_RESCUE_EFFECT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_RESCUE_EFFECT_NOTIFICATION_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomBubble: 4AAFA7 / 6D97D0; helper precedes victim, unlike the request.
struct GameBubbleRescueEffectNotification {
  static constexpr std::uint16_t kMessageId = 0x9588;
  std::uint32_t helper_object_id = 0;
  std::uint32_t actor_object_id = 0;
  std::uint8_t kind = 0;
  TcpPacket Encode() const;
  static GameBubbleRescueEffectNotification Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleRescueEffectNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_RESCUE_EFFECT_NOTIFICATION_H_
