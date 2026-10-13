#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "features/rooms/room_game_session.h"
#include "protocol/battle/game_over_notification.h"
#include "protocol/battle/game_result_notification.h"
#include "protocol/battle/game_result_presentation_notification.h"
#include "protocol/battle/game_return_to_room_notification.h"
#include "protocol/battle/game_score_update_notification.h"
#include "protocol/battle/game_start_notification.h"

namespace bubble_fighter::server::features::rooms {
bool RoomGameSession::Ended() const noexcept {
  return phase_ == RoomGamePhase::kAwaitingReports ||
         phase_ == RoomGamePhase::kResult ||
         phase_ == RoomGamePhase::kResultPresentation ||
         phase_ == RoomGamePhase::kReturning;
}

void RoomGameSession::AppendScore(std::vector<RoomGameDelivery>& deliveries,
                                  ScoreEvent event, std::uint32_t source_slot,
                                  std::uint32_t target_slot) {
  auto& source = members_.at(source_slot);
  auto& target = members_.at(target_slot);
  if (source.team == 0 || target.team == 0) {
    return;
  }  // Requires trusted roster binding.
  auto first = source.scores;
  auto second = target.scores;
  auto team = team_scores_[source.team];
  const auto increment = [](std::uint16_t& value) {
    if (value != (std::numeric_limits<std::uint16_t>::max)()) {
      ++value;
    }
  };
  const auto index = event == ScoreEvent::kBubble   ? 0U
                     : event == ScoreEvent::kPang   ? 1U
                     : event == ScoreEvent::kRescue ? 2U
                                                    : 3U;
  constexpr std::array<std::uint8_t, 4> keys{0, 12, 5, 14};
  increment(first[index]);
  protocol::GameScoreUpdateNotification update;
  update.first_slot = static_cast<std::uint8_t>(source_slot);
  update.first_updates.push_back({keys[index], first[index]});
  if (event == ScoreEvent::kPang) {
    increment(second[3]);
    increment(team);
    update.group_index = source.team;
    update.group_value = team;
    update.second_slot = static_cast<std::uint8_t>(target_slot);
    update.second_updates.push_back({keys[3], second[3]});
  }
  // Encode/allocate before committing counters. A confirmed opponent pang
  // gives one team point; this is explicit local scoring, not an original
  // server reward formula. The uint16 wire ceiling saturates, never wraps.
  deliveries.push_back({update.Encode(), {}, {}});
  source.scores = first;
  if (event == ScoreEvent::kPang) {
    target.scores = second;
    team_scores_[source.team] = team;
  }
}

void RoomGameSession::ClearInteractions() noexcept {
  for (auto& [slot, member] : members_) {
    (void)slot;
    member.loading_data.reset();
    member.loaded.reset();
    member.damagers.clear();
    member.movement.Clear();
    member.damage_sources.clear();
    member.pop.reset();
    member.action.reset();
    member.stomp.reset();
    member.rescue.reset();
    member.rescue_effect.reset();
  }
}

void RoomGameSession::RequireSettlement() {
  if (phase_ != RoomGamePhase::kLoading) {
    throw std::logic_error("Late settlement requirement.");
  }
  settlement_required_ = true;
}

void RoomGameSession::ConfirmSettlement(Time now) {
  if (phase_ != RoomGamePhase::kResult || !settlement_required_ ||
      settlement_confirmed_) {
    throw std::logic_error("Unexpected settlement confirmation.");
  }
  Observe(now);
  settlement_confirmed_ = true;
  deadline_ = now + kResultDisplay;
}

std::array<std::uint16_t, 4> RoomGameSession::FinalScores(
    std::uint32_t slot) const {
  if (phase_ != RoomGamePhase::kResult) {
    throw std::logic_error("Scores are not frozen.");
  }
  return members_.at(slot).scores;
}

std::array<std::uint16_t, 2> RoomGameSession::FinalTeamScores() const {
  if (phase_ != RoomGamePhase::kResult) {
    throw std::logic_error("Team scores are not frozen.");
  }
  return {team_scores_[1], team_scores_[2]};
}

void RoomGameSession::Finish() noexcept {
  phase_ = RoomGamePhase::kReturning;
  ClearInteractions();
  for (auto& [slot, member] : members_) {
    (void)slot;
    member.ending_report.reset();
    member.scores = {};
  }
  team_scores_ = {};
}

std::vector<RoomGameDelivery> RoomGameSession::Poll(Time now) {
  Observe(now);
  if (now < deadline_ || phase_ == RoomGamePhase::kReturning) {
    return {};
  }
  if (phase_ == RoomGamePhase::kPlaying) {
    std::vector<RoomGameDelivery> output{
        {protocol::GameOverNotification{
             protocol::GameOverNotification::kTimeOver, 0}
             .Encode(),
         {},
         {}}};
    phase_ = RoomGamePhase::kAwaitingReports;
    game_over_at_ = now;
    deadline_ = now + kReportLimit;
    ClearInteractions();
    return output;
  }
  if (phase_ == RoomGamePhase::kAwaitingReports) {
    // 9E5E70/428B90 initialize winner=-1; S985 carries our actual team
    // totals. 42A340 compares that first column for ordinary mode 1.
    // These zero ancillary fields are NOT an explicit draw/winner.
    std::vector<RoomGameDelivery> output{
        {protocol::GameResultNotification{0, duration_, 0, 0}.Encode(),
         {},
         {}}};
    phase_ = RoomGamePhase::kResult;
    deadline_ = now + kResultDisplay;
    return output;
  }
  if (phase_ == RoomGamePhase::kResult) {
    if (settlement_required_ && !settlement_confirmed_) {
      return {};
    }
    std::vector<RoomGameDelivery> output{
        {protocol::GameResultPresentationNotification{0}.Encode(), {}, {}}};
    phase_ = RoomGamePhase::kResultPresentation;
    deadline_ = now + kResultPresentationDisplay;
    return output;
  }
  if (phase_ == RoomGamePhase::kResultPresentation) {
    // Deliver to the old game context before RoomRegistry replaces it
    // with S117. Reversing this order would clean the new room context.
    std::vector<RoomGameDelivery> output{
        {protocol::GameReturnToRoomNotification{}.Encode(), {}, {}}};
    Finish();
    return output;
  }
  if (phase_ != RoomGamePhase::kCountdown) {
    // Loading timeout or completed local result display returns to waiting.
    Finish();
    return {};
  }
  std::vector<RoomGameDelivery> output{
      {protocol::GameStartNotification{duration_, seed_}.Encode(), {}, {}}};
  phase_ = RoomGamePhase::kPlaying;
  deadline_ = now + std::chrono::milliseconds{duration_};
  return output;
}
}  // namespace bubble_fighter::server::features::rooms
