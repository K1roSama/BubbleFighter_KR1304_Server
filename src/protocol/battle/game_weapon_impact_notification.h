#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_WEAPON_IMPACT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_WEAPON_IMPACT_NOTIFICATION_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 706840 writer / 6F4EF0 reader, 24 bytes.
struct GameWeaponImpactNotification {
  static constexpr std::uint16_t kMessageId = 0x8215;
  std::uint32_t damager_key = 0;
  std::array<std::uint32_t, 3> position_bits{};
  // 7966C0 copies GoDamager+656; +692 supplies DamagerKey.
  std::uint32_t source_object_id = 0;
  std::uint32_t target_object_id = 0;
  TcpPacket Encode() const;
  static GameWeaponImpactNotification Decode(const TcpPacket& packet);
  bool operator==(const GameWeaponImpactNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_WEAPON_IMPACT_NOTIFICATION_H_
