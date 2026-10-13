#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_LOADING_COMPLETE_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_LOADING_COMPLETE_REQUEST_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E069: GameStage update -> 0x0075B3E0 -> 0x006DC970, after loading.
// The last two fields are zero in that writer; their semantics are unknown.
// Codecs preserve them and do not authorize a transition into a game.
struct GameLoadingCompleteRequest {
  static constexpr std::uint16_t kMessageId = 578;
  std::uint32_t delay_milliseconds = 0;
  std::uint32_t value_at4 = 0;
  std::uint32_t value_at8 = 0;
  std::uint32_t value_at12 = 0;

  TcpPacket Encode() const;
  static GameLoadingCompleteRequest Decode(const TcpPacket& packet);
  bool operator==(const GameLoadingCompleteRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_LOADING_COMPLETE_REQUEST_H_
