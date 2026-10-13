#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESULT_PRESENTATION_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESULT_PRESENTATION_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E072: 0x009E323D reads one byte and passes bit 0 to 0x009E9EF0.
// Preserve raw flags; only the consumer of bit 0 has been identified.
struct GameResultPresentationNotification {
  static constexpr std::uint16_t kMessageId = 615;
  std::uint8_t flags = 0;
  TcpPacket Encode() const;
  static GameResultPresentationNotification Decode(const TcpPacket& packet);
  bool operator==(const GameResultPresentationNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RESULT_PRESENTATION_NOTIFICATION_H_
