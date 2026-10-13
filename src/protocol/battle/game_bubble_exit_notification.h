#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_EXIT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_EXIT_NOTIFICATION_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomBubble: 6E2FD0 / 6F7B30; self exit uses source FFFFFFFF or FFFFFFFE.
struct GameBubbleExitNotification {
  static constexpr std::uint16_t kMessageId = 0x9580;
  std::uint32_t actor_object_id = 0;
  std::uint32_t source_object_id = 0;
  TcpPacket Encode() const;
  static GameBubbleExitNotification Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleExitNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_EXIT_NOTIFICATION_H_
