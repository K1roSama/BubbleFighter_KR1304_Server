#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_MELEE_ATTACK_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_MELEE_ATTACK_NOTIFICATION_H_
#include <vector>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 8ECB00 -> 6E2990, reader 6F2700; 19 + 4*n bytes.
struct GameMeleeAttackNotification {
  static constexpr std::uint16_t kMessageId = 0x81FE;
  static constexpr std::size_t kMaxValues =
      (TcpPacketCodec::kDefaultMaxPayloadSize - 19) / 4;
  std::uint32_t actor_object_id = 0;
  std::uint16_t weapon_id = 0;
  // 8ECB00 obtains this from actor+548, not the selected weapon at+872.
  // Preserve the native byte until its complete semantics are confirmed.
  std::uint8_t byte_at6 = 0;
  std::uint32_t shot_sequence = 0;
  std::uint32_t packed_time = 0;
  std::vector<std::uint32_t> values;
  TcpPacket Encode() const;
  static GameMeleeAttackNotification Decode(const TcpPacket& packet);
  bool operator==(const GameMeleeAttackNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_MELEE_ATTACK_NOTIFICATION_H_
