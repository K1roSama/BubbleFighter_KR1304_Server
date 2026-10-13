#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_HIT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_HIT_NOTIFICATION_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E087: 19 bytes, distinct damager lookup key (not ActorObjectId).
struct GameHitNotification {
  static constexpr std::uint16_t kMessageId = 0x958B;
  std::uint32_t damager_key = 0;
  std::uint32_t target_object_id = 0;
  std::array<std::uint16_t, 3> position{};  // Binary16 bits, preserved.
  std::uint8_t hit_kind = 0;
  // Native consumer treats any nonzero uint32 as true. Preserve the
  // received value; the observed sender narrows its input to uint8.
  std::uint32_t keep_damager_value = 0;
  TcpPacket Encode() const;
  static GameHitNotification Decode(const TcpPacket& packet);
  bool operator==(const GameHitNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_HIT_NOTIFICATION_H_
