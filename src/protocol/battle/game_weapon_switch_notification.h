#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_WEAPON_SWITCH_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_WEAPON_SWITCH_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 6E32D0 -> 6F65A0. Unlike the reference server's sample,
// this sample writes six DWORDs, including the final byte widened to DWORD.
struct GameWeaponSwitchNotification {
  static constexpr std::uint16_t kMessageId = 0x802A;
  std::uint32_t weapon_id = 0;
  std::uint32_t weapon_index = 0;
  std::uint32_t actor_object_id = 0;
  std::uint32_t time_value = 0;
  std::uint32_t select_value = 0;
  std::uint32_t value_at20 = 0;
  TcpPacket Encode() const;
  static GameWeaponSwitchNotification Decode(const TcpPacket& packet);
  bool operator==(const GameWeaponSwitchNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_WEAPON_SWITCH_NOTIFICATION_H_
