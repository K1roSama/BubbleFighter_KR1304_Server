#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_NOTIFICATION_H_
#include "protocol/battle/game_score_record.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E073: S614 reader 0x010D16A0. Full layout, no runtime reward/exit policy.
struct GameScoreNotification {
  static constexpr std::uint16_t kMessageId = 614;
  static constexpr std::size_t kMaxPayloadBytes =
      GameScoreRecord::kMaxPayloadBytes;
  static constexpr std::size_t kMaxEntries =
      1024;  // Local limit, not original server policy.
  GameScoreRecord scores;
  std::uint32_t value_at19180 = 0;
  std::uint32_t elapsed_milliseconds = 0;
  std::map<std::uint32_t, std::array<std::uint32_t, 2>> entries_at19188;
  std::uint8_t byte_at19196 = 0;
  std::uint8_t byte_at19197 = 0;
  std::uint16_t flags_at19272 = 0;
  // Wire order is client object offsets 19200, 19248, 19224.
  std::array<std::u16string, 3> texts_at19200;
  std::uint8_t extension_tag = 0;
  // Only tag 0x3A carries these three uint32 values.
  std::array<std::uint32_t, 3> values_at19276{};
  TcpPacket Encode() const;
  static GameScoreNotification Decode(const TcpPacket& packet);
  bool operator==(const GameScoreNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_NOTIFICATION_H_
