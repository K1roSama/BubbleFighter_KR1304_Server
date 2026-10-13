#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_GRENADE_DETONATE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_GRENADE_DETONATE_NOTIFICATION_H_
#include <array>
#include <vector>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 6DF330 -> 708820, reader 6CAB40: 52 + 4*n bytes.
struct GameGrenadeDetonateNotification {
  static constexpr std::uint16_t kMessageId = 0x8084;
  static constexpr std::size_t kMaxValues =
      (TcpPacketCodec::kDefaultMaxPayloadSize - 52) / 4;
  std::uint32_t source_damager_key = 0;
  std::array<std::uint32_t, 3> position_bits{};
  // Two further native vectors; preserve their roles without guessing.
  std::array<std::uint32_t, 6> vector_bits{};
  std::uint32_t damager_key = 0;
  std::uint32_t packed_time = 0;
  std::vector<std::uint32_t> values;
  TcpPacket Encode() const;
  static GameGrenadeDetonateNotification Decode(const TcpPacket& packet);
  bool operator==(const GameGrenadeDetonateNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_GRENADE_DETONATE_NOTIFICATION_H_
