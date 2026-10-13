#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_POP_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_POP_NOTIFICATION_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomBubble: 6E3F00 / 6FAC30; flags stay full uint32 on the wire.
struct GameBubblePopNotification {
  static constexpr std::uint16_t kMessageId = 0x9585;
  std::uint32_t apply_value = 0;
  std::uint32_t actor_object_id = 0;
  std::uint32_t source_object_id = 0;
  std::array<std::uint32_t, 3> vector_bits{};
  std::uint32_t contact_value = 0;
  std::uint32_t reason_value = 0;
  TcpPacket Encode() const;
  static GameBubblePopNotification Decode(const TcpPacket& packet);
  bool operator==(const GameBubblePopNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_POP_NOTIFICATION_H_
