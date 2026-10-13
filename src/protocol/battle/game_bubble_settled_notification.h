#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_SETTLED_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_SETTLED_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 6E4D60 -> 6D55D0 -> Character virtual +552/4A6F90.
// Completes the rolling entry animation and requests native state 301.
struct GameBubbleSettledNotification {
  static constexpr std::uint16_t kMessageId = 0x9589;
  std::uint32_t actor_object_id = 0;
  TcpPacket Encode() const;
  static GameBubbleSettledNotification Decode(const TcpPacket& packet);
  bool operator==(const GameBubbleSettledNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_BUBBLE_SETTLED_NOTIFICATION_H_
