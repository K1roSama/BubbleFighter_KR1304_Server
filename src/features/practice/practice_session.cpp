#include "features/practice/practice_session.h"

#include "protocol/battle/game_countdown_notification.h"
#include "protocol/battle/game_over_notification.h"
#include "protocol/battle/game_respawn_notification.h"
#include "protocol/battle/game_result_notification.h"
#include "protocol/battle/game_start_notification.h"
#include "protocol/unresolved/server_to_client_message_0x025e.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <bcrypt.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace bubble_fighter::server::features::practice {
PracticeSession::PracticeSession(Time now, std::uint32_t seed)
    : last_observed_(now), deadline_(now + kLoadLimit), seed_(seed) {
  // Avoid abs(INT_MIN) and degenerate boundary seeds in the client's RNG.
  if (seed == 0 || seed >= 0x7FFFFFFF) {
    throw std::invalid_argument("Invalid local practice random seed.");
  }
}

std::uint32_t PracticeSession::CreateRandomSeed() {
  for (;;) {
    std::uint32_t value = 0;
    if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&value),
                        sizeof(value), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
      throw std::runtime_error("Practice random seed generation failed.");
    }
    value &= 0x7FFFFFFF;
    if (value != 0 && value != 0x7FFFFFFF) {
      return value;
    }
  }
}

void PracticeSession::Observe(Time now) {
  if (now < last_observed_) {
    throw std::runtime_error("Practice clock moved backwards.");
  }
  last_observed_ = now;
}

std::vector<protocol::TcpPacket> PracticeSession::LoadingComplete(
    const protocol::GameLoadingCompleteRequest& report, Time now) {
  Observe(now);
  if (report.value_at8 != 0 || report.value_at12 != 0 ||
      (report.delay_milliseconds != 0 && report.delay_milliseconds != 3000)) {
    throw std::runtime_error("Unsupported practice loading report.");
  }
  if (report_) {
    if (*report_ != report) {
      throw std::runtime_error("Changed duplicate practice loading report.");
    }
    return {};  // No clock, actor or seed reset in countdown or play.
  }
  if (phase_ != PracticePhase::kLoading || now >= deadline_) {
    throw std::runtime_error("Late practice loading report.");
  }
  // S606 read/discard field uses the documented local zero baseline.
  std::vector<protocol::TcpPacket> packets{
      protocol::ServerToClientMessage0x025E{0}.Encode(),
      protocol::GameCountdownNotification{
          static_cast<std::uint32_t>(kCountdown.count())}
          .Encode()};
  report_ = report;
  phase_ = PracticePhase::kCountdown;
  deadline_ = now + kCountdown;
  return packets;
}

std::vector<protocol::TcpPacket> PracticeSession::Poll(Time now) {
  Observe(now);
  if (now < deadline_) {
    return {};
  }
  if (phase_ == PracticePhase::kFinishing) {
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             game_over_at_ - started_at_)
                             .count();
    const auto bounded = std::min<std::int64_t>(
        elapsed, (std::numeric_limits<std::uint32_t>::max)());
    // E075: mode 40 has its own result image/telemetry path. The first
    // word is unused by this reader's consumers. Group result bytes
    // retain the native zero baseline for this unranked local practice;
    // neither comes from the 29 untrusted report counters. This is not
    // an original-server reward/result policy. See PracticeResultFlow.md.
    std::vector<protocol::TcpPacket> packets{protocol::GameResultNotification{
        0, static_cast<std::uint32_t>(bounded), 0, 0}
                                                 .Encode()};
    phase_ = PracticePhase::kResult;
    deadline_ = now + kResultDisplay;
    return packets;
  }
  if (phase_ == PracticePhase::kPlaying) {
    // E072: reason 2 is the native Time Over branch. In non-Zombie
    // mode 40 that branch does not use the second outcome argument;
    // zero is an explicit ignored-argument policy, not a winner code.
    std::vector<protocol::TcpPacket> packets{protocol::GameOverNotification{
        protocol::GameOverNotification::kTimeOver, 0}
                                                 .Encode()};
    phase_ = PracticePhase::kAwaitingReport;
    last_reported_movement_.reset();
    ClearActionReports();
    game_over_at_ = now;
    deadline_ = now + kReportLimit;
    return packets;
  }
  if (phase_ != PracticePhase::kCountdown) {
    return {};
  }
  std::vector<protocol::TcpPacket> packets{protocol::GameStartNotification{
      static_cast<std::uint32_t>(kPlayLimit.count()), seed_}
                                               .Encode()};
  phase_ = PracticePhase::kPlaying;
  started_at_ = now;
  deadline_ =
      now + kPlayLimit;  // Starts when S608 is prepared, never before it.
  return packets;
}

bool PracticeSession::Expired(Time now) {
  Observe(now);
  return (phase_ == PracticePhase::kLoading ||
          phase_ == PracticePhase::kAwaitingReport ||
          phase_ == PracticePhase::kResult) &&
         now >= deadline_;
}

void PracticeSession::SubmitReport(const protocol::GameReportRequest& report,
                                   Time now) {
  Observe(now);
  if (report.random_seed != seed_) {
    throw std::runtime_error("Practice report belongs to another start.");
  }
  if ((phase_ == PracticePhase::kFinishing ||
       phase_ == PracticePhase::kResult) &&
      ending_report_) {
    if (*ending_report_ != report) {
      throw std::runtime_error("Changed duplicate practice game report.");
    }
    return;  // No delay extension, statistics, inventory or reward write.
  }
  if (phase_ != PracticePhase::kAwaitingReport || now >= deadline_) {
    throw std::runtime_error(
        "Practice report outside the server-initiated game-over window.");
  }
  // Public seed binds an already owned connection; it never authenticates
  // the client or validates the other 29 client-supplied counters.
  ending_report_ = report;
  phase_ = PracticePhase::kFinishing;
  deadline_ = game_over_at_ + kGameOverDisplay;
}

std::vector<protocol::TcpPacket> PracticeSession::ReportActorDeath(
    const protocol::GameActorDeathNotification& report, Time now) {
  CheckActorWindow(report.actor_object_id, now);
  if (actor_phase_ != ActorPhase::kAlive) {
    return {};
  }
  // Local unranked practice accepts the assigned player's own report;
  // it does not validate client physics or grant persistent rewards.
  std::vector<protocol::TcpPacket> packets{report.Encode()};
  actor_phase_ = ActorPhase::kDown;
  respawn_completed_ = false;
  return packets;
}

void PracticeSession::CheckActorWindow(std::uint32_t actor_object_id,
                                       Time now) {
  Observe(now);
  if (actor_object_id != kLocalPlayerObjectId ||
      phase_ != PracticePhase::kPlaying || now >= deadline_) {
    throw std::runtime_error(
        "Actor report outside the owned active practice scene.");
  }
}

std::vector<protocol::TcpPacket> PracticeSession::ReportRespawnWait(
    const protocol::GameActorRespawnWaitNotification& report, Time now) {
  CheckActorWindow(report.actor_object_id, now);
  if (actor_phase_ == ActorPhase::kAlive) {
    throw std::runtime_error("Respawn wait requires a current death report.");
  }
  if (actor_phase_ != ActorPhase::kDown) {
    return {};
  }
  std::vector<protocol::TcpPacket> packets{report.Encode()};
  actor_phase_ = ActorPhase::kWaiting;
  return packets;
}

std::vector<protocol::TcpPacket> PracticeSession::RequestRespawn(
    const protocol::GameRespawnRequest& request, Time now) {
  CheckActorWindow(request.parameters.actor_object_id, now);
  // Native mode-40 default sender omits both optional values. The
  // current GameOption.xml specifies itemResetOnRespawn=0. Alternate
  // spawn selection and revive items require separate evidence/policy.
  if (request.parameters.value_at4 || request.parameters.value_at12 ||
      request.reset_items_value != 0) {
    throw std::runtime_error("Unsupported local practice respawn parameters.");
  }
  if (actor_phase_ == ActorPhase::kAwaitingCompletion) {
    return {};
  }
  if (actor_phase_ != ActorPhase::kWaiting) {
    throw std::runtime_error(
        "Respawn request outside the death/wait sequence.");
  }
  // Preserve the member +304 cleared baseline (0x009E1D70). E083
  // confirms zero passes the mode-40 predicate for either context flag.
  // This is an explicit local policy, not an inferred original life count.
  std::vector<protocol::TcpPacket> packets{
      protocol::GameRespawnNotification{request.parameters, 0, 0}.Encode()};
  actor_phase_ = ActorPhase::kAwaitingCompletion;
  last_reported_movement_
      .reset();  // A respawn invalidates the old observed position.
  return packets;
}

std::vector<protocol::TcpPacket> PracticeSession::CompleteRespawn(
    const protocol::GameRespawnCompletionNotification& report, Time now) {
  CheckActorWindow(report.parameters.actor_object_id, now);
  if (report.parameters.value_at4 || report.parameters.value_at12 ||
      report.byte_at20 != 0 || report.value_at21 != 0) {
    throw std::runtime_error("Unsupported local practice respawn completion.");
  }
  if (actor_phase_ == ActorPhase::kAlive && respawn_completed_) {
    return {};
  }
  if (actor_phase_ != ActorPhase::kAwaitingCompletion) {
    throw std::runtime_error("Unsolicited practice respawn completion.");
  }
  // The native completion applies actor callbacks, including clearing
  // the fall/death suppression byte. Forward once before resetting the
  // server guard, so a later death can start a fresh cycle.
  std::vector<protocol::TcpPacket> packets{report.Encode()};
  actor_phase_ = ActorPhase::kAlive;
  respawn_completed_ = true;
  return packets;
}

PracticePhase PracticeSession::Phase() const noexcept { return phase_; }

bool PracticeSession::CanForwardShot(const protocol::CompressedShotRecord& shot,
                                     Time now) {
  Observe(now);
  if (shot.actor_object_id != kLocalPlayerObjectId || shot.weapon_index >= 6 ||
      !shot.HasFiniteCoordinates()) {
    throw std::runtime_error("Invalid owned practice shot.");
  }
  // E086: current local-character sender routes to TCP; receiving the
  // shot creates the objects. Forward only within this unranked scene.
  // In-flight shots after death/game over cannot revive or prolong play.
  return phase_ == PracticePhase::kPlaying && now < deadline_ &&
         actor_phase_ == ActorPhase::kAlive;
}

std::vector<protocol::TcpPacket> PracticeSession::SubmitShot(
    const protocol::GameShotNotification& report, Time now) {
  if (!CanForwardShot(report.shot, now)) {
    return {};
  }
  return {report.Encode()};
}

std::vector<protocol::TcpPacket> PracticeSession::SubmitShot(
    const protocol::GameExtendedShotNotification& report, Time now) {
  // Encode first to validate sizes even for direct callers/late reports.
  auto packet = report.Encode();
  if (!CanForwardShot(report.shot, now)) {
    return {};
  }
  return {std::move(packet)};
}

bool PracticeSession::CanRetainActionReport(Time now) {
  Observe(now);
  // Local, unranked observation only. Early reports cannot start play;
  // in-flight reports after the deadline cannot extend the session.
  if ((phase_ != PracticePhase::kLoading &&
       phase_ != PracticePhase::kCountdown &&
       phase_ != PracticePhase::kPlaying) ||
      now >= deadline_) {
    ClearActionReports();
    return false;
  }
  return true;
}

void PracticeSession::SubmitActionReport(
    const protocol::ClientToServerMessage0x0260& report, Time now) {
  if (CanRetainActionReport(now)) {
    last_action_report0x0260_ = report;
  }
}

void PracticeSession::SubmitActionReport(
    const protocol::ClientToServerMessage0x07ED& report, Time now) {
  if (CanRetainActionReport(now)) {
    last_action_report0x07_ed_ = report;
  }
  // E085: event 41 precedes 0x9582 on the ordinary local fall path.
  // Receiving the report does not substitute for that death message or
  // permit direct C602. Mode 15's direct path is not mode 40.
}

const std::optional<protocol::ClientToServerMessage0x0260>&
PracticeSession::LastActionReport0x0260() const noexcept {
  return last_action_report0x0260_;
}

const std::optional<protocol::ClientToServerMessage0x07ED>&
PracticeSession::LastActionReport0x07ED() const noexcept {
  return last_action_report0x07_ed_;
}

void PracticeSession::ClearActionReports() noexcept {
  last_action_report0x0260_.reset();
  last_action_report0x07_ed_.reset();
}

void PracticeSession::SubmitMovement(
    const protocol::GameMovementNotification& report, Time now) {
  Observe(now);
  if (report.actor_object_id != kLocalPlayerObjectId) {
    throw std::runtime_error(
        "Movement does not belong to the assigned practice player.");
  }
  for (const auto value : report.position) {
    if (!std::isfinite(value)) {
      throw std::runtime_error("Non-finite practice movement position.");
    }
  }
  // E084: the native timer predicate is false for an unstarted timer,
  // so movement alone cannot prove S608 has occurred. Retain one report
  // during loading/countdown/play; never infer object/death authority.
  // Late in-flight movement is discarded after game over or a deadline.
  if ((phase_ != PracticePhase::kLoading &&
       phase_ != PracticePhase::kCountdown &&
       phase_ != PracticePhase::kPlaying) ||
      now >= deadline_) {
    last_reported_movement_.reset();
    return;
  }
  last_reported_movement_ = report;
  // No echo: 0x00703270 skips applying movement to a locally owned actor.
  // ValueAt20 is not used as a server clock/sequence or lifetime extension.
}

const std::optional<protocol::GameMovementNotification>&
PracticeSession::LastReportedMovement() const noexcept {
  return last_reported_movement_;
}

void PracticeSession::CheckLoadingReportWindow(bool duplicate, Time now) {
  Observe(now);
  if ((phase_ != PracticePhase::kLoading &&
       (!duplicate || (phase_ != PracticePhase::kCountdown &&
                       phase_ != PracticePhase::kPlaying))) ||
      now >= deadline_) {
    throw std::runtime_error(
        "Practice loading data outside its local acceptance window.");
  }
}

void PracticeSession::SubmitLoadingData(
    const protocol::ClientToServerMessage0x0243& report, Time now) {
  CheckLoadingReportWindow(loading_data_.has_value(), now);
  if (loading_data_) {
    if (*loading_data_ != report) {
      throw std::runtime_error("Changed duplicate practice loading data.");
    }
    return;
  }
  (void)report.Encode();  // Also bound callers that bypass wire decoding.
  loading_data_ = report;
  // Explicit unranked local policy: record only. Client map/spawn values
  // cannot create authoritative objects, rewards or extend any deadline.
}

void PracticeSession::SubmitLoadingActor(
    const protocol::ClientToServerMessage0x025F& report, Time now) {
  if (report.value_at0 != kLocalPlayerObjectId) {
    throw std::runtime_error(
        "Loading actor does not match the assigned local practice slot.");
  }
  CheckLoadingReportWindow(loading_actor_.has_value(), now);
  if (loading_actor_ && *loading_actor_ != report) {
    throw std::runtime_error("Changed duplicate practice loading actor.");
  }
  // Confirm only the expected loading observation. This does not prove
  // client physics/death state or authorize a respawn or reward.
  loading_actor_ = report;
}
}  // namespace bubble_fighter::server::features::practice
