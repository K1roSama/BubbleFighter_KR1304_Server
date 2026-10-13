#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_PLAYER_UPDATE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_PLAYER_UPDATE_NOTIFICATION_H_
#include <map>

#include "protocol/battle/game_score_value_update.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E074: S655 map of native slot -> ordered value updates. The native map
// reader rejects duplicate slots. Repeated field indices remain ordered.
struct GameScorePlayerUpdateNotification {
  static constexpr std::uint16_t kMessageId = 655;
  static constexpr std::size_t kMaxPlayers = 36;
  static constexpr std::size_t kMaxPayloadBytes =
      4 + 8 * kMaxPlayers + 3 * GameScoreValueUpdate::kMaxTotalEntries;
  std::map<std::uint32_t, std::vector<GameScoreValueUpdate>> players;
  TcpPacket Encode() const;
  static GameScorePlayerUpdateNotification Decode(const TcpPacket& packet);
  bool operator==(const GameScorePlayerUpdateNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_PLAYER_UPDATE_NOTIFICATION_H_
