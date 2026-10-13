#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_ACTION_START_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_ACTION_START_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 6E4600 -> 6D55D0 -> actor +508 (8F3BD0).
// The first field owns the action; the second is the bubbled target.
struct GameBubbleActionStartNotification {
  static constexpr std::uint16_t kMessageId = 0x8017;
  std::uint32_t actor_object_id = 0;
  std::uint32_t target_object_id = 0;
  std::uint32_t variant = 0;
  TcpPacket Encode() const;
  static GameBubbleActionStartNotification Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleActionStartNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_ACTION_START_NOTIFICATION_H_
