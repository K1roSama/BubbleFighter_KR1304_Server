#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_CONTACT_POP_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_CONTACT_POP_REQUEST_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomBubble: 8EE685 writer / 6FA9A0; second actor is the contact initiator.
struct GameBubbleContactPopRequest {
  static constexpr std::uint16_t kMessageId = 0x9583;
  std::uint32_t actor_object_id = 0;
  std::uint32_t source_object_id = 0;
  TcpPacket Encode() const;
  static GameBubbleContactPopRequest Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleContactPopRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_CONTACT_POP_REQUEST_H_
