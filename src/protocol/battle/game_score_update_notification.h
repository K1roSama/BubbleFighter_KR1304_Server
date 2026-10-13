#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_UPDATE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_UPDATE_NOTIFICATION_H_
#include "protocol/battle/game_score_value_update.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E074: S985, 0x009E3420 -> 0x009E8B20. Values replace selected fields;
// they are not increments. No runtime score or reward authority is implied.
struct GameScoreUpdateNotification {
  static constexpr std::uint16_t kMessageId = 985;
  static constexpr std::size_t kMaxPayloadBytes =
      17 + 3 * GameScoreValueUpdate::kMaxTotalEntries;
  std::uint8_t group_index =
      0;  // 0 skips the first group-array write; 1..36 selects an entry.
  std::uint16_t group_value = 0;
  std::uint8_t first_slot = 0xFF;
  std::uint8_t second_slot = 0xFF;
  // Signed-negative slot bytes are ignored by the client. Preserve their
  // raw representation and their lists; do not infer a player identity.
  std::vector<GameScoreValueUpdate> first_updates;
  std::vector<GameScoreValueUpdate> second_updates;
  std::vector<GameScoreValueUpdate>
      group_updates;  // Indices 0..36, used in modes 5/6.
  TcpPacket Encode() const;
  static GameScoreUpdateNotification Decode(const TcpPacket& packet);
  bool operator==(const GameScoreUpdateNotification&) const = default;

 private:
  void Validate() const;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_UPDATE_NOTIFICATION_H_
