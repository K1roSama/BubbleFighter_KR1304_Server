#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_EXTENDED_SHOT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_EXTENDED_SHOT_NOTIFICATION_H_
#include "protocol/battle/compressed_shot_record.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct GameExtendedShotNotification {
  static constexpr std::uint16_t kMessageId = 0x820F;
  static constexpr std::size_t kMaxValues =
      (TcpPacketCodec::kDefaultMaxPayloadSize - 40) / 4;
  CompressedShotRecord shot;
  std::uint32_t value_at40 = 0;
  std::vector<std::uint32_t> values_at44;
  TcpPacket Encode() const;
  static GameExtendedShotNotification Decode(const TcpPacket& packet);
  bool operator==(const GameExtendedShotNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_EXTENDED_SHOT_NOTIFICATION_H_
