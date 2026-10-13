#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_RESCUE_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_RESCUE_REQUEST_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomBubble: 6E40C0 / 6D97D0; kind is a byte, not a boolean.
struct GameBubbleRescueRequest {
  static constexpr std::uint16_t kMessageId = 0x8200;
  std::uint32_t actor_object_id = 0;
  std::uint32_t helper_object_id = 0;
  std::uint8_t kind = 0;
  TcpPacket Encode() const;
  static GameBubbleRescueRequest Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleRescueRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_RESCUE_REQUEST_H_
