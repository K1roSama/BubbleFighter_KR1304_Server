#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_WEAPON_PRESET_CHANGE_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_WEAPON_PRESET_CHANGE_REQUEST_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct WeaponPresetChangeRequest {
  static constexpr std::uint16_t kMessageId = 832;
  static constexpr std::uint8_t kMaxPresetIndex = 4;
  static constexpr std::uint32_t kMenuObjectId = 0xFFFFFFFF;

  // The menu writer supplies MenuObjectId. Battle supplies a local object
  // ID; decoding that ID does not establish ownership or authorize play.
  std::uint32_t object_id = kMenuObjectId;
  std::uint8_t preset_index = 0;

  TcpPacket Encode() const;
  static WeaponPresetChangeRequest Decode(const TcpPacket& packet);
  bool operator==(const WeaponPresetChangeRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_WEAPON_PRESET_CHANGE_REQUEST_H_
