#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_PLAZA_MOVEMENT_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_PLAZA_MOVEMENT_H_
#include <array>
#include <cstdint>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 913640 writes C511: 4+12+1+1+4 bytes. 9D9930
// case 514 reads the same fields preceded by the trusted player ID.
struct PlazaMovementRequest {
  static constexpr std::uint16_t kMessageId = 511;
  std::uint32_t sequence = 0;
  std::array<std::uint32_t, 3> position_bits{};
  std::array<std::uint8_t, 2> axes{};
  std::uint32_t yaw_bits = 0;
  void Validate() const;
  TcpPacket Encode() const;
  static PlazaMovementRequest Decode(const TcpPacket& packet);
  bool operator==(const PlazaMovementRequest&) const = default;
};
struct PlazaMovementNotification {
  static constexpr std::uint16_t kMessageId = 514;
  std::uint32_t player_id = 0;
  PlazaMovementRequest movement;
  TcpPacket Encode() const;
  static PlazaMovementNotification Decode(const TcpPacket& packet);
  bool operator==(const PlazaMovementNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_PLAZA_MOVEMENT_H_
