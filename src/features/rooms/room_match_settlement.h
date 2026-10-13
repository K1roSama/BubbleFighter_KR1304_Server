#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_MATCH_SETTLEMENT_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_MATCH_SETTLEMENT_H_
#include <functional>

#include "features/characters/character_store.h"
#include "features/rooms/match_reward_policy.h"
#include "protocol/battle/game_score_notification.h"

namespace bubble_fighter::server::features::rooms {
struct RoomMatchMember {
  std::uint32_t slot = 0;
  std::uint64_t account_id = 0, character_id = 0;
  std::uint8_t team = 0;
  // Bubble, pang, help, death: accepted server transitions only.
  std::array<std::uint16_t, 4> scores{};
  bool operator==(const RoomMatchMember&) const = default;
};
struct RoomMatchRequest {
  std::array<std::uint8_t, 16> match_id{};
  MatchRewardPolicy rules;
  std::uint32_t duration_milliseconds = 0;
  std::array<std::uint16_t, 2> team_scores{};
  std::vector<RoomMatchMember> members;
  std::uint32_t format_version =
      2;  // V1 receipts retain their original no-awards presentation.
  void Validate() const;
  std::uint8_t Winner() const noexcept;
  // Versioned LOCAL durable request, not a game protocol message.
  std::vector<std::uint8_t> Encode() const;
  static RoomMatchRequest Decode(std::span<const std::uint8_t> data);
};
struct RoomMatchReward {
  std::uint32_t slot = 0;
  characters::CharacterRecord character;
  characters::CharacterStatistics before;
  bool operator==(const RoomMatchReward&) const = default;
};
struct RoomMatchResult {
  std::vector<RoomMatchReward> rewards;
  void Validate(const RoomMatchRequest& request) const;
  std::vector<std::uint8_t> Encode() const;
  static RoomMatchResult Decode(std::span<const std::uint8_t> data);
  protocol::GameScoreNotification Scores(const RoomMatchRequest& request) const;
  std::vector<std::pair<std::uint32_t, protocol::TcpPacket>> Profiles(
      const RoomMatchRequest& request) const;
  bool operator==(const RoomMatchResult&) const = default;
};
using RoomMatchSettler =
    std::function<RoomMatchResult(const RoomMatchRequest&)>;
std::array<std::uint8_t, 16> CreateRoomMatchId();
}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_MATCH_SETTLEMENT_H_
