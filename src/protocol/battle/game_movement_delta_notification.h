#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_MOVEMENT_DELTA_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_MOVEMENT_DELTA_NOTIFICATION_H_
#include <array>
#include <optional>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E084: 0x809B's mask controls three float32 and three byte updates.
// The original reader ignores high mask bits; preserve them on the wire.
// Current solo practice selects full records and does not apply deltas.
struct GameMovementDeltaNotification {
  static constexpr std::uint16_t kMessageId = 0x809B;
  std::uint32_t actor_object_id = 0;
  std::uint16_t value_at4 = 0;
  std::uint8_t mask = 0;
  std::array<std::optional<float>, 3> position{};
  std::array<std::optional<std::uint8_t>, 3> angles_and_flags{};
  TcpPacket Encode() const;
  static GameMovementDeltaNotification Decode(const TcpPacket& packet);
  bool operator==(const GameMovementDeltaNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_MOVEMENT_DELTA_NOTIFICATION_H_
