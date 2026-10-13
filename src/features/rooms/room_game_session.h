#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_GAME_SESSION_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_GAME_SESSION_H_
#include <array>
#include <chrono>
#include <map>
#include <optional>
#include <set>
#include <span>

#include "features/battle/combat_movement_timeline.h"
#include "features/battle/combat_scene.h"
#include "features/battle/fighter_attribute_catalog.h"
#include "features/battle/fighter_maximum_health.h"
#include "protocol/battle/game_loading_complete_request.h"
#include "protocol/battle/game_report_request.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/unresolved/client_to_server_message_0x0243.h"

namespace bubble_fighter::server::features::rooms {
enum class RoomGamePhase {
  kLoading,
  kCountdown,
  kPlaying,
  kAwaitingReports,
  kResult,
  kResultPresentation,
  kReturning
};
enum class RoomActorPhase {
  kAlive,
  kBubble,
  kDown,
  kWaiting,
  kAwaitingCompletion
};
struct RoomGameDelivery {
  protocol::TcpPacket packet;
  std::optional<std::uint32_t> only_slot;
  std::optional<std::uint32_t> except_slot;
};

// Owned under the registry lock. All members share the same deadlines and seed.
class RoomGameSession {
 public:
  using Clock = std::chrono::steady_clock;
  using Time = Clock::time_point;
  static constexpr auto kLoadLimit = std::chrono::seconds{90};
  // 69C710 displays floor(remaining / 1000): 4000 yields 3, 2, 1 after the
  // first tick.
  static constexpr auto kCountdown = std::chrono::milliseconds{4000};
  // Local bounded presentation/report policy, not recovered server timers.
  static constexpr auto kReportLimit = std::chrono::seconds{10};
  static constexpr auto kGameOverDisplay = std::chrono::seconds{3};
  static constexpr auto kResultDisplay = std::chrono::seconds{3};
  // Native S615 starts a four-second transition. Allow ten further
  // seconds for the panel; the total return deadline is local policy.
  static constexpr auto kResultPresentationDisplay = std::chrono::seconds{14};
  // Local maximum delay for a victim's result after an accepted hit.
  // This is a correlation bound, not a native projectile lifetime.
  static constexpr auto kDamageResultLifetime = std::chrono::seconds{5};
  RoomGameSession(std::span<const std::uint32_t> slots, Time now,
                  std::uint32_t seed, std::uint32_t duration_milliseconds);
  RoomGamePhase Phase() const noexcept { return phase_; }
  std::vector<RoomGameDelivery> Process(std::uint32_t slot,
                                        const protocol::TcpPacket& packet,
                                        Time now);
  std::vector<RoomGameDelivery> Poll(Time now);
  void Finish() noexcept;
  // Trusted S117 equipment snapshot, installed by RoomRegistry before
  // loading. This is never populated from a gameplay packet.
  void BindEquipment(std::uint32_t slot, std::span<const std::uint32_t> items);
  void BindTeam(std::uint32_t slot, std::uint8_t team);
  void BindMap(std::u16string_view trusted_map_key);
  bool HasStaticCollision() const noexcept { return scene_ != nullptr; }
  void BindCharacter(std::uint32_t slot, std::uint32_t trusted_character_type);
  const std::array<std::optional<battle::FighterBaseAttributes>, 3>&
  BaseAttributes(std::uint32_t slot) const;
  const std::optional<battle::FighterBaseAttributes>& SelectedBaseAttributes(
      std::uint32_t slot) const;
  // Initial ordinary-mode maximum, including resource equipment effects.
  // Not a mutable HP ledger and not a promise that skills are implemented.
  std::optional<float> InitialMaximumHealth(std::uint32_t slot) const;
  void RequireSettlement();
  void ConfirmSettlement(Time now);
  std::array<std::uint16_t, 4> FinalScores(std::uint32_t slot) const;
  std::array<std::uint16_t, 2> FinalTeamScores() const;
  static bool Handles(std::uint16_t message_id) noexcept;
  static std::uint32_t ActorId(std::uint32_t slot);

 private:
  enum class DamagerKind { kBullet, kMelee, kGrenade, kScatter };
  struct Damager {
    std::uint32_t weapon_id = 0;
    std::uint16_t hit_slots = 0;
    battle::CombatVector origin{};
    Time created{};
    std::optional<battle::CombatVector> reported_source_position;
    std::array<std::uint64_t, 16> target_movement_epochs{};
    DamagerKind kind = DamagerKind::kBullet;
  };
  struct Member {
    struct BubbleAction {
      std::uint32_t target_slot, variant;
      Time expires;
      // Native action 110 visits nearby enemies, not only the clicked
      // target. Each current bubble may respond once per action.
      std::array<std::uint64_t, 16> target_epochs{};
      std::uint16_t eligible_targets = 0, requested_targets = 0;
      // 8018 ends the hit window, but 8045 may still finish the
      // actor animation. Keep that authorization until completion.
      bool ended = false;
    };
    struct Interaction {
      std::uint32_t source_slot;
      std::uint8_t kind;
      Time expires;
    };
    std::uint8_t team = 0;
    // S985 keys 0/12/5/14: bubble, pang, help and death. Counts derive
    // only from accepted room transitions, never C103 client counters.
    std::array<std::uint16_t, 4> scores{};
    std::optional<protocol::GameReportRequest> ending_report;
    std::optional<Interaction> pop, rescue, rescue_effect;
    std::optional<BubbleAction> action;
    struct StompAction {
      std::uint32_t target_slot;
      std::uint64_t target_epoch;
      Time expires;
      bool hit = false;
    };
    std::optional<StompAction> stomp;
    std::optional<protocol::ClientToServerMessage0x0243> loading_data;
    std::optional<protocol::GameLoadingCompleteRequest> loaded;
    bool actor_reported = false;
    RoomActorPhase actor_phase = RoomActorPhase::kAlive;
    bool bubble_settled = false;
    bool respawn_completed = false;
    // BFCLIENT02 10EA690/10EAB10/817E80 built-in empty-slot fallback.
    std::array<std::uint32_t, 6> weapons{2006, 1005, 7001, 5033, 0, 0};
    std::uint32_t next_damager_key = 0;
    // One bit per already-hit player; no unbounded event history.
    std::map<std::uint32_t, Damager> damagers;
    battle::CombatMovementTimeline movement;
    std::uint64_t movement_epoch = 0;
    std::array<std::optional<battle::FighterBaseAttributes>, 3> base_attributes;
    std::optional<std::uint32_t> character_type;
    std::vector<std::uint32_t> equipment_items;
    std::optional<float> initial_maximum_hp;
    // Bounded by the participants and their four trusted gun slots.
    // Correlates a victim report even after its projectile retires.
    // This is not an HP or physical-collision authority.
    std::map<std::pair<std::uint32_t, std::uint32_t>, Time> damage_sources;
  };
  void Observe(Time now);
  void CheckActor(std::uint32_t slot, std::uint32_t actor) const;
  std::vector<RoomGameDelivery> ProcessActor(std::uint32_t slot,
                                             const protocol::TcpPacket& packet);
  std::vector<RoomGameDelivery> ProcessHit(std::uint32_t slot,
                                           const protocol::TcpPacket& packet);
  std::vector<RoomGameDelivery> ProcessDamageResult(
      std::uint32_t slot, const protocol::TcpPacket& packet);
  std::vector<RoomGameDelivery> ProcessBubble(
      std::uint32_t slot, const protocol::TcpPacket& packet);
  std::vector<RoomGameDelivery> ProcessBubbleAction(
      std::uint32_t slot, const protocol::TcpPacket& packet);
  std::vector<RoomGameDelivery> ProcessActorAction(
      std::uint32_t slot, const protocol::TcpPacket& packet);
  std::vector<RoomGameDelivery> ProcessMeleeAttack(
      std::uint32_t slot, const protocol::TcpPacket& packet);
  std::vector<RoomGameDelivery> ProcessWeaponImpact(
      std::uint32_t slot, const protocol::TcpPacket& packet);
  std::vector<RoomGameDelivery> ProcessGrenadeDetonate(
      std::uint32_t slot, const protocol::TcpPacket& packet);
  std::vector<RoomGameDelivery> ProcessStomp(std::uint32_t slot,
                                             const protocol::TcpPacket& packet);
  void ClearBubbleLinks(std::uint32_t slot) noexcept;
  static void RefreshInitialMaximumHealth(Member& member);
  static void ResetMovement(Member& member) noexcept;
  enum class ScoreEvent { kBubble, kPang, kRescue, kDeath };
  void AppendScore(std::vector<RoomGameDelivery>& deliveries, ScoreEvent event,
                   std::uint32_t source_slot, std::uint32_t target_slot);
  bool Ended() const noexcept;
  void ClearInteractions() noexcept;
  std::map<std::uint32_t, Member> members_;
  RoomGamePhase phase_ = RoomGamePhase::kLoading;
  Time last_observed_, deadline_;
  Time game_over_at_{};
  std::array<std::uint16_t, 3> team_scores_{};
  std::uint32_t seed_, duration_;
  bool settlement_required_ = false, settlement_confirmed_ = false;
  const battle::CombatScene* scene_ = nullptr;
};
}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_GAME_SESSION_H_
