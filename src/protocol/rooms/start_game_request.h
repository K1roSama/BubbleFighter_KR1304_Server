#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_START_GAME_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_START_GAME_REQUEST_H_

#include <array>
#include <cstdint>
#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R21: block from 0x00AADD50 (map rules), written field by
// field by 0x010D26C0. Opaque here; 0x009A0880 overrides one value with a
// time limit for modes 8/28/43 (180000) and 32 (600000).
struct StartGameRules {
  std::array<std::uint32_t, 13> values{};
  std::uint8_t byte_at52 = 0;
  bool operator==(const StartGameRules&) const = default;
};

// RoomCreation R21: C503, the start request of ordinary rooms. Start
// button 0x00970590 -> 0x0096DD20 -> 0x009A0880 -> writer 0x009D7570.
// Round-battle rooms (option 0x1, mode != 19) send C504 instead.
struct StartGameRequest {
  static constexpr std::uint16_t kMessageId = 0x01F7;

  // Byte argument of 0x009A0880 (0 from the start button), zero-extended.
  std::uint32_t variant = 0;
  // 0x010E48A0: map resource resolved from the room's map selection.
  std::u16string map_resource;
  // 0x010E49C0: a value read from that resource.
  std::u16string map_attribute;
  // Copy of room context +184, the selected map key.
  std::u16string map_key;
  // 0x0041B060(0, 1, 0); meaning unknown.
  std::uint32_t value_at76 = 0;
  StartGameRules rules;
  // Cleared by 0x0099B890; 0x009A0880 can only set bit 0.
  std::uint32_t flags = 0;

  TcpPacket Encode() const;
  static StartGameRequest Decode(const TcpPacket& packet);
  bool operator==(const StartGameRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_START_GAME_REQUEST_H_
