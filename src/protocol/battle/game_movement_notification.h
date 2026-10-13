#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_MOVEMENT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_MOVEMENT_NOTIFICATION_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E084: complete WanderInfo, 21 wire bytes. At offsets refer to the
// client's 22-byte record, which has padding before its last uint16.
struct GameMovementNotification {
  static constexpr std::uint16_t kMessageId = 0x8075;
  std::uint32_t actor_object_id = 0;
  std::array<float, 3> position{};
  std::uint8_t angle_at16 = 0;
  std::uint8_t angle_at17 = 0;
  std::uint8_t flags_at18 = 0;
  std::uint16_t value_at20 = 0;
  TcpPacket Encode() const;
  static GameMovementNotification Decode(const TcpPacket& packet);
  bool operator==(const GameMovementNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_MOVEMENT_NOTIFICATION_H_
