#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_DEFLECT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_DEFLECT_NOTIFICATION_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomBubble: 6E43D0 / 6FB680; mode 1 emits FFFFFFFF at offset 20.
struct GameBubbleDeflectNotification {
  static constexpr std::uint16_t kMessageId = 0x9584;
  std::uint32_t actor_object_id = 0;
  std::uint32_t source_object_id = 0;
  std::array<std::uint32_t, 3> vector_bits{};
  std::uint32_t value_at20 = 0;
  std::uint32_t contact_value = 0;
  TcpPacket Encode() const;
  static GameBubbleDeflectNotification Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleDeflectNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_DEFLECT_NOTIFICATION_H_
