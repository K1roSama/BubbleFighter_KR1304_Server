#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_SHOT_POP_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_SHOT_POP_REQUEST_H_
#include <array>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// Legacy type name: 6E42B0 / 6FAAF0 also handles ordinary action-110
// click-to-pop (8F0EF0), sent by SourceObjectId, contact value 0.
// First actor is the bubbled target. See BattlePop.md counterevidence.
struct GameBubbleShotPopRequest {
  static constexpr std::uint16_t kMessageId = 0x957F;
  std::uint32_t actor_object_id = 0;
  std::uint32_t source_object_id = 0;
  TcpPacket Encode() const;
  static GameBubbleShotPopRequest Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleShotPopRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_SHOT_POP_REQUEST_H_
