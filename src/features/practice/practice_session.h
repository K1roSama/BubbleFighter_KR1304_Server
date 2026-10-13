#ifndef BUBBLEFIGHTER_SERVER_FEATURES_PRACTICE_PRACTICE_SESSION_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_PRACTICE_PRACTICE_SESSION_H_
#include <chrono>
#include <optional>

#include "protocol/battle/game_actor_death_notification.h"
#include "protocol/battle/game_actor_respawn_wait_notification.h"
#include "protocol/battle/game_extended_shot_notification.h"
#include "protocol/battle/game_loading_complete_request.h"
#include "protocol/battle/game_movement_notification.h"
#include "protocol/battle/game_report_request.h"
#include "protocol/battle/game_respawn_completion_notification.h"
#include "protocol/battle/game_respawn_request.h"
#include "protocol/battle/game_shot_notification.h"
#include "protocol/unresolved/client_to_server_message_0x0243.h"
#include "protocol/unresolved/client_to_server_message_0x025f.h"
#include "protocol/unresolved/client_to_server_message_0x0260.h"
#include "protocol/unresolved/client_to_server_message_0x07ed.h"

namespace bubble_fighter::server::features::practice {
enum class PracticePhase {
  kLoading,
  kCountdown,
  kPlaying,
  kAwaitingReport,
  kFinishing,
  kResult
};

class PracticeSession {
 public:
  using Clock = std::chrono::steady_clock;
  using Time = Clock::time_point;
  // E081: SceneRoster::ComposePractice fixes the local participant to
  // slot 0. Native 0x006D2040 resets that slot's object allocation range
  // and takes its first ID for ChrLocalPlayer. This is not CharacterId.
  static constexpr std::uint32_t kLocalPlayerObjectId = 1'000'000;
  // Explicit local practice policy, not reconstructed Nexon durations.
  static constexpr auto kLoadLimit = std::chrono::seconds{90};
  static constexpr auto kCountdown = std::chrono::milliseconds{3000};
  static constexpr auto kPlayLimit = std::chrono::milliseconds{240000};
  static constexpr auto kReportLimit = std::chrono::seconds{10};
  static constexpr auto kGameOverDisplay = std::chrono::seconds{3};
  static constexpr auto kResultDisplay = std::chrono::seconds{3};
  PracticeSession(Time now, std::uint32_t seed);
  static std::uint32_t CreateRandomSeed();
  std::vector<protocol::TcpPacket> LoadingComplete(
      const protocol::GameLoadingCompleteRequest& report, Time now);
  std::vector<protocol::TcpPacket> Poll(Time now);
  void SubmitReport(const protocol::GameReportRequest& report, Time now);
  void SubmitLoadingData(const protocol::ClientToServerMessage0x0243& report,
                         Time now);
  void SubmitLoadingActor(const protocol::ClientToServerMessage0x025F& report,
                          Time now);
  std::vector<protocol::TcpPacket> ReportActorDeath(
      const protocol::GameActorDeathNotification& report, Time now);
  std::vector<protocol::TcpPacket> ReportRespawnWait(
      const protocol::GameActorRespawnWaitNotification& report, Time now);
  std::vector<protocol::TcpPacket> RequestRespawn(
      const protocol::GameRespawnRequest& request, Time now);
  std::vector<protocol::TcpPacket> CompleteRespawn(
      const protocol::GameRespawnCompletionNotification& report, Time now);
  void SubmitMovement(const protocol::GameMovementNotification& report,
                      Time now);
  std::vector<protocol::TcpPacket> SubmitShot(
      const protocol::GameShotNotification& report, Time now);
  std::vector<protocol::TcpPacket> SubmitShot(
      const protocol::GameExtendedShotNotification& report, Time now);
  const std::optional<protocol::GameMovementNotification>&
  LastReportedMovement() const noexcept;
  void SubmitActionReport(const protocol::ClientToServerMessage0x0260& report,
                          Time now);
  void SubmitActionReport(const protocol::ClientToServerMessage0x07ED& report,
                          Time now);
  const std::optional<protocol::ClientToServerMessage0x0260>&
  LastActionReport0x0260() const noexcept;
  const std::optional<protocol::ClientToServerMessage0x07ED>&
  LastActionReport0x07ED() const noexcept;
  bool Expired(Time now);
  PracticePhase Phase() const noexcept;

 private:
  void Observe(Time now);
  void CheckLoadingReportWindow(bool duplicate, Time now);
  void CheckActorWindow(std::uint32_t actor_object_id, Time now);
  bool CanRetainActionReport(Time now);
  void ClearActionReports() noexcept;
  bool CanForwardShot(const protocol::CompressedShotRecord& shot, Time now);
  enum class ActorPhase { kAlive, kDown, kWaiting, kAwaitingCompletion };
  PracticePhase phase_ = PracticePhase::kLoading;
  Time last_observed_;
  Time deadline_;
  Time game_over_at_{};
  Time started_at_{};
  std::uint32_t seed_;
  std::optional<protocol::GameLoadingCompleteRequest> report_;
  std::optional<protocol::GameReportRequest> ending_report_;
  std::optional<protocol::ClientToServerMessage0x0243> loading_data_;
  std::optional<protocol::ClientToServerMessage0x025F> loading_actor_;
  ActorPhase actor_phase_ = ActorPhase::kAlive;
  bool respawn_completed_ = false;
  std::optional<protocol::GameMovementNotification> last_reported_movement_;
  std::optional<protocol::ClientToServerMessage0x0260>
      last_action_report0x0260_;
  std::optional<protocol::ClientToServerMessage0x07ED>
      last_action_report0x07_ed_;
};
}  // namespace bubble_fighter::server::features::practice

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_PRACTICE_PRACTICE_SESSION_H_
