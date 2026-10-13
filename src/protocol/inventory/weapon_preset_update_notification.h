#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_WEAPON_PRESET_UPDATE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_WEAPON_PRESET_UPDATE_NOTIFICATION_H_

#include <array>
#include <cstdint>
#include <vector>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E052: current message 837, not legacy 835. Records are three uint32s,
// not the legacy pair of bytes followed by two uint32s.
struct WeaponPresetUpdateNotification {
  static constexpr std::uint16_t kMessageId = 0x0345;
  static constexpr std::uint32_t kMaxCollectionEntries = 1024;

  std::uint8_t byte_at0 = 0;
  std::uint32_t player_id = 0;
  std::vector<std::array<std::uint32_t, 3>> records;
  std::vector<std::uint8_t> bytes;

  TcpPacket Encode() const;
  static WeaponPresetUpdateNotification Decode(const TcpPacket& packet);
  bool operator==(const WeaponPresetUpdateNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_WEAPON_PRESET_UPDATE_NOTIFICATION_H_
