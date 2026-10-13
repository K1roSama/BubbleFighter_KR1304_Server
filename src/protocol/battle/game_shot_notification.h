#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SHOT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SHOT_NOTIFICATION_H_
#include "protocol/battle/compressed_shot_record.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct GameShotNotification {
  static constexpr std::uint16_t kMessageId = 0x809D;
  CompressedShotRecord shot;
  TcpPacket Encode() const;
  static GameShotNotification Decode(const TcpPacket& packet);
  bool operator==(const GameShotNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SHOT_NOTIFICATION_H_
