#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_WEAPON_PRESET_CHANGE_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_WEAPON_PRESET_CHANGE_RESPONSE_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct WeaponPresetChangeResponse {
  static constexpr std::uint16_t kMessageId = 836;
  static constexpr std::uint32_t kSuccess = 0;

  std::uint8_t preset_index = 0;
  // Only zero's success meaning is established. Preserve other raw codes
  // for analysis, without inventing individual failure meanings.
  std::uint32_t result = kSuccess;

  TcpPacket Encode() const;
  static WeaponPresetChangeResponse Decode(const TcpPacket& packet);
  bool operator==(const WeaponPresetChangeResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_WEAPON_PRESET_CHANGE_RESPONSE_H_
