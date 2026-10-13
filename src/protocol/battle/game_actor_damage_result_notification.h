#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_ACTOR_DAMAGE_RESULT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_ACTOR_DAMAGE_RESULT_NOTIFICATION_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomDamageRelay: 6E3B80 -> 6F8280, emitted after HP reaches zero. The
// consumer selects bubble or direct death. This codec alone grants no game
// state.
struct GameActorDamageResultNotification {
  static constexpr std::uint16_t kMessageId = 0x958A;
  std::uint32_t actor_object_id = 0;
  std::uint32_t source_object_id = 0;
  // Three float32 bit patterns. Impact direction/position distinction
  // depends on the damager branch, so preserve the original vector.
  std::array<std::uint32_t, 3> vector_bits{};
  std::uint32_t weapon_id = 0;
  std::uint32_t direct_death_value = 0;
  std::uint32_t full_health_hit_value = 0;
  // Writer replaces the low 24 bits with elapsed time; the upper byte
  // retains stack data. The reader consumes all four bytes.
  std::uint32_t time_value = 0;
  std::uint32_t value_at36 = 0;
  TcpPacket Encode() const;
  static GameActorDamageResultNotification Decode(const TcpPacket& packet);
  bool operator==(const GameActorDamageResultNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_ACTOR_DAMAGE_RESULT_NOTIFICATION_H_
