#include "features/rooms/room_game_session.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "features/battle/combat_geometry.h"
#include "features/battle/combat_scene.h"
#include "features/battle/fighter_attribute_catalog.h"
#include "features/battle/fighter_maximum_health.h"
#include "features/battle/weapon_catalog.h"
#include "features/inventory/item_catalog.h"
#include "protocol/battle/game_actor_damage_result_notification.h"
#include "protocol/battle/game_actor_death_notification.h"
#include "protocol/battle/game_actor_respawn_wait_notification.h"
#include "protocol/battle/game_bubble_action_complete_notification.h"
#include "protocol/battle/game_bubble_action_end_notification.h"
#include "protocol/battle/game_bubble_action_start_notification.h"
#include "protocol/battle/game_bubble_contact_pop_request.h"
#include "protocol/battle/game_bubble_deflect_notification.h"
#include "protocol/battle/game_bubble_exit_notification.h"
#include "protocol/battle/game_bubble_pop_notification.h"
#include "protocol/battle/game_bubble_rescue_effect_notification.h"
#include "protocol/battle/game_bubble_rescue_notification.h"
#include "protocol/battle/game_bubble_rescue_request.h"
#include "protocol/battle/game_bubble_settled_notification.h"
#include "protocol/battle/game_bubble_shot_pop_request.h"
#include "protocol/battle/game_countdown_notification.h"
#include "protocol/battle/game_damager_remove_notification.h"
#include "protocol/battle/game_extended_shot_notification.h"
#include "protocol/battle/game_grenade_detonate_notification.h"
#include "protocol/battle/game_hit_notification.h"
#include "protocol/battle/game_loading_complete_request.h"
#include "protocol/battle/game_melee_attack_notification.h"
#include "protocol/battle/game_movement_notification.h"
#include "protocol/battle/game_report_request.h"
#include "protocol/battle/game_respawn_completion_notification.h"
#include "protocol/battle/game_respawn_request.h"
#include "protocol/battle/game_shot_notification.h"
#include "protocol/battle/game_stomp_hit_notification.h"
#include "protocol/battle/game_stomp_start_notification.h"
#include "protocol/battle/game_weapon_impact_notification.h"
#include "protocol/battle/game_weapon_switch_notification.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/unresolved/client_to_server_message_0x0243.h"
#include "protocol/unresolved/client_to_server_message_0x025f.h"
#include "protocol/unresolved/client_to_server_message_0x0260.h"
#include "protocol/unresolved/client_to_server_message_0x07ed.h"
#include "protocol/unresolved/client_to_server_message_0x800b.h"
#include "protocol/unresolved/client_to_server_message_0x800c.h"
#include "protocol/unresolved/client_to_server_message_0x800d.h"
#include "protocol/unresolved/client_to_server_message_0x800e.h"
#include "protocol/unresolved/client_to_server_message_0x802d.h"
#include "protocol/unresolved/server_to_client_message_0x025e.h"

namespace bubble_fighter::server::features::rooms {
RoomGameSession::RoomGameSession(std::span<const std::uint32_t> slots, Time now,
                                 std::uint32_t seed,
                                 std::uint32_t duration_milliseconds)
    : last_observed_(now),
      deadline_(now + kLoadLimit),
      seed_(seed),
      duration_(duration_milliseconds) {
  if (slots.empty() || seed == 0 || seed >= 0x7FFFFFFF || duration_ == 0) {
    throw std::invalid_argument("Invalid room game initialization.");
  }
  for (auto slot : slots) {
    ActorId(slot);
    if (!members_.emplace(slot, Member{}).second) {
      throw std::invalid_argument("Duplicate game slot.");
    }
    members_.at(slot).next_damager_key = slot * 100000U;
  }
}

void RoomGameSession::BindEquipment(std::uint32_t slot,
                                    std::span<const std::uint32_t> items) {
  if (phase_ != RoomGamePhase::kLoading || members_.at(slot).loaded) {
    throw std::runtime_error("Cannot replace equipment after room loading.");
  }
  std::array<std::uint32_t, 6> weapons{2006, 1005, 7001, 5033, 0, 0};
  std::uint32_t occupied = 0;
  for (auto item : items) {
    const auto* definition = inventory::ItemCatalog::Find(item);
    if (!definition) {
      throw std::runtime_error("Unknown trusted room equipment.");
    }
    // 817680 builds the slot map; 817E80 reads entries 18..21.
    for (std::uint32_t index = 0; index < 4; ++index) {
      const auto bit = 1U << (index + 18);
      if ((definition->slot_mask & bit) == 0) {
        continue;
      }
      if ((occupied & bit) != 0) {
        throw std::runtime_error("Conflicting trusted room weapons.");
      }
      occupied |= bit;
      const auto* weapon = battle::WeaponCatalog::FindEquipmentItem(item);
      weapons[index] =
          weapon && weapon->weapon_slot_index == index ? weapon->weapon_id : 0;
    }
  }
  auto& member = members_.at(slot);
  member.weapons = weapons;
  member.equipment_items.assign(items.begin(), items.end());
  RefreshInitialMaximumHealth(member);
}

void RoomGameSession::BindTeam(std::uint32_t slot, std::uint8_t team) {
  if (phase_ != RoomGamePhase::kLoading || members_.at(slot).loaded ||
      (team != 1 && team != 2)) {
    throw std::runtime_error("Invalid trusted room team binding.");
  }
  members_.at(slot).team = team;
}

void RoomGameSession::BindMap(std::u16string_view trusted_map_key) {
  if (phase_ != RoomGamePhase::kLoading ||
      std::any_of(members_.begin(), members_.end(), [](const auto& entry) {
        return entry.second.loaded.has_value();
      })) {
    throw std::runtime_error("Cannot replace room map after loading.");
  }
  scene_ = battle::CombatScene::Find(trusted_map_key);
}

void RoomGameSession::BindCharacter(std::uint32_t slot,
                                    std::uint32_t trusted_character_type) {
  auto& member = members_.at(slot);
  if (phase_ != RoomGamePhase::kLoading || member.loaded) {
    throw std::runtime_error(
        "Cannot replace fighter attributes after room loading.");
  }
  // Current room policy is channel 60 / ordinary mode 1. Preserve all
  // variants for exact resource comparisons; selection is below.
  for (std::size_t i = 0; i < member.base_attributes.size(); ++i) {
    member.base_attributes[i] = battle::FighterAttributeCatalog::Resolve(
        trusted_character_type, 60, 1,
        static_cast<battle::FighterAttributeVariant>(i));
  }
  member.character_type = trusted_character_type;
  RefreshInitialMaximumHealth(member);
}

const std::array<std::optional<battle::FighterBaseAttributes>, 3>&
RoomGameSession::BaseAttributes(std::uint32_t slot) const {
  return members_.at(slot).base_attributes;
}

const std::optional<battle::FighterBaseAttributes>&
RoomGameSession::SelectedBaseAttributes(std::uint32_t slot) const {
  // CA3A60 reads Experimental.arenaTestSeason.use=false from the pinned
  // resource. 47F1A0 therefore keeps actor+860 (normal table). Current
  // RoomCreation rejects nonzero mode flags, including BubbleCup 0x10.
  // This accessor deliberately remains the unmodified base table.
  return members_.at(slot).base_attributes[static_cast<std::size_t>(
      battle::FighterAttributeVariant::kNormal)];
}

void RoomGameSession::RefreshInitialMaximumHealth(Member& member) {
  member.initial_maximum_hp.reset();
  const auto& base = member.base_attributes[static_cast<std::size_t>(
      battle::FighterAttributeVariant::kNormal)];
  if (!member.character_type || !base) {
    return;
  }
  const auto equipment = battle::FighterMaximumHealth::Equipment(
      *member.character_type, member.equipment_items, 60);
  // Current ordinary mode 1 / modeFlags=0 has no initial actor+1444
  // multiplier. character.lua returns the supplied 1.0 outside the arena
  // and test-season branches. Runtime effects must not reuse this seed.
  member.initial_maximum_hp =
      battle::FighterMaximumHealth::Ordinary(base->hp, equipment, 1.0F, 1.0F);
}

std::optional<float> RoomGameSession::InitialMaximumHealth(
    std::uint32_t slot) const {
  return members_.at(slot).initial_maximum_hp;
}

void RoomGameSession::ResetMovement(Member& member) noexcept {
  member.movement.Clear();
  ++member.movement_epoch;
}

std::uint32_t RoomGameSession::ActorId(std::uint32_t slot) {
  if (slot >= 16) {
    throw std::invalid_argument("Unsupported game slot.");
  }
  // BFCLIENT02 6D2040 resets and consumes the slot's first object id.
  return 1000000U + slot * 100000U;
}

void RoomGameSession::Observe(Time now) {
  if (now < last_observed_) {
    throw std::runtime_error("Room game clock moved backwards.");
  }
  last_observed_ = now;
}

void RoomGameSession::CheckActor(std::uint32_t slot,
                                 std::uint32_t actor) const {
  if (!members_.contains(slot) || actor != ActorId(slot)) {
    throw std::runtime_error(
        "Game actor does not belong to the authenticated room slot.");
  }
}

bool RoomGameSession::Handles(std::uint16_t id) noexcept {
  using namespace protocol;
  return id == GameLoadingCompleteRequest::kMessageId ||
         id == ClientToServerMessage0x0243::kMessageId ||
         id == ClientToServerMessage0x025F::kMessageId ||
         id == ClientToServerMessage0x0260::kMessageId ||
         id == ClientToServerMessage0x07ED::kMessageId ||
         id == GameMovementNotification::kMessageId ||
         id == GameShotNotification::kMessageId ||
         id == GameExtendedShotNotification::kMessageId ||
         id == GameActorDeathNotification::kMessageId ||
         id == GameActorRespawnWaitNotification::kMessageId ||
         id == GameRespawnRequest::kMessageId ||
         id == GameRespawnCompletionNotification::kMessageId ||
         id == GameHitNotification::kMessageId ||
         id == GameDamagerRemoveNotification::kMessageId ||
         id == GameActorDamageResultNotification::kMessageId ||
         id == GameBubbleExitNotification::kMessageId ||
         id == GameBubbleSettledNotification::kMessageId ||
         id == GameWeaponSwitchNotification::kMessageId ||
         id == GameBubbleActionStartNotification::kMessageId ||
         id == GameBubbleActionEndNotification::kMessageId ||
         id == GameBubbleActionCompleteNotification::kMessageId ||
         id == GameMeleeAttackNotification::kMessageId ||
         id == GameWeaponImpactNotification::kMessageId ||
         id == GameGrenadeDetonateNotification::kMessageId ||
         id == GameStompStartNotification::kMessageId ||
         id == GameStompHitNotification::kMessageId ||
         id == ClientToServerMessage0x800B::kMessageId ||
         id == ClientToServerMessage0x800C::kMessageId ||
         id == ClientToServerMessage0x800D::kMessageId ||
         id == ClientToServerMessage0x800E::kMessageId ||
         id == ClientToServerMessage0x802D::kMessageId ||
         id == GameBubbleShotPopRequest::kMessageId ||
         id == GameBubbleContactPopRequest::kMessageId ||
         id == GameBubbleDeflectNotification::kMessageId ||
         id == GameBubblePopNotification::kMessageId ||
         id == GameBubbleRescueRequest::kMessageId ||
         id == GameBubbleRescueNotification::kMessageId ||
         id == GameBubbleRescueEffectNotification::kMessageId ||
         id == GameReportRequest::kMessageId;
}

std::vector<RoomGameDelivery> RoomGameSession::Process(
    std::uint32_t slot, const protocol::TcpPacket& packet, Time now) {
  using namespace protocol;
  Observe(now);
  auto found = members_.find(slot);
  if (found == members_.end()) {
    throw std::runtime_error("Game message by a nonparticipant.");
  }
  const bool awaiting_commit = phase_ == RoomGamePhase::kResult &&
                               settlement_required_ && !settlement_confirmed_;
  if (phase_ != RoomGamePhase::kReturning && !awaiting_commit &&
      now >= deadline_) {
    throw std::runtime_error(
        "Game message after phase deadline; poll transition first.");
  }
  auto& member = found->second;
  if (packet.message_id == GameReportRequest::kMessageId) {
    const auto report = GameReportRequest::Decode(packet);
    // Late reports from a closed game are parsed but cannot reopen it.
    if (phase_ == RoomGamePhase::kReturning) {
      return {};
    }
    if (report.random_seed != seed_) {
      throw std::runtime_error("Room report belongs to another start.");
    }
    if (member.ending_report) {
      if (*member.ending_report != report) {
        throw std::runtime_error("Changed duplicate room game report.");
      }
      return {};
    }
    if (phase_ == RoomGamePhase::kResult ||
        phase_ == RoomGamePhase::kResultPresentation) {
      return {};
    }  // A late first report cannot change the result.
    if (phase_ != RoomGamePhase::kAwaitingReports) {
      throw std::runtime_error(
          "Room report outside the server-initiated game-over window.");
    }
    member.ending_report = report;
    if (std::all_of(members_.begin(), members_.end(), [](const auto& entry) {
          return entry.second.ending_report.has_value();
        })) {
      deadline_ = std::max(now, game_over_at_ + kGameOverDisplay);
    }
    return {};  // Public seed and 29 opaque counters authorize no rewards.
  }
  if (packet.message_id == ClientToServerMessage0x0243::kMessageId) {
    auto data = ClientToServerMessage0x0243::Decode(packet);
    if (Ended()) {
      return {};
    }
    if (member.loading_data) {
      if (*member.loading_data != data) {
        throw std::runtime_error("Changed room loading snapshot.");
      }
    } else {
      if (phase_ != RoomGamePhase::kLoading) {
        throw std::runtime_error("Late room loading snapshot.");
      }
      member.loading_data = std::move(data);
    }
    return {};
  }
  if (packet.message_id == ClientToServerMessage0x025F::kMessageId) {
    CheckActor(slot, ClientToServerMessage0x025F::Decode(packet).value_at0);
    if (Ended()) {
      return {};
    }
    if (!member.actor_reported && phase_ != RoomGamePhase::kLoading) {
      throw std::runtime_error("Late room actor report.");
    }
    member.actor_reported = true;
    return {};
  }
  if (packet.message_id == GameLoadingCompleteRequest::kMessageId) {
    auto report = GameLoadingCompleteRequest::Decode(packet);
    // 6DC970: ordinary mode 1 emits 0 or the per-player 3000 branch.
    if (report.value_at8 || report.value_at12 ||
        (report.delay_milliseconds != 0 && report.delay_milliseconds != 3000)) {
      throw std::runtime_error("Unsupported ordinary-room loading report.");
    }
    if (Ended()) {
      return {};
    }
    if (member.loaded) {
      if (*member.loaded != report) {
        throw std::runtime_error("Changed duplicate room loading report.");
      }
      return {};
    }
    if (phase_ != RoomGamePhase::kLoading) {
      throw std::runtime_error("Late room loading completion.");
    }
    std::vector<RoomGameDelivery> output{
        {ServerToClientMessage0x025E{0}.Encode(), slot, {}}};
    const bool others_loaded = std::all_of(
        members_.begin(), members_.end(), [slot](const auto& entry) {
          return entry.first == slot || entry.second.loaded.has_value();
        });
    if (others_loaded) {
      output.push_back({GameCountdownNotification{
                            static_cast<std::uint32_t>(kCountdown.count())}
                            .Encode(),
                        {},
                        {}});
    }
    member.loaded = report;
    if (others_loaded) {
      phase_ = RoomGamePhase::kCountdown;
      deadline_ = now + kCountdown;
    }
    return output;
  }
  if (packet.message_id == ClientToServerMessage0x0260::kMessageId) {
    ClientToServerMessage0x0260::Decode(packet);
    return {};
  }
  if (packet.message_id == ClientToServerMessage0x07ED::kMessageId) {
    ClientToServerMessage0x07ED::Decode(packet);
    return {};
  }
  if (packet.message_id == GameMovementNotification::kMessageId) {
    const auto move = GameMovementNotification::Decode(packet);
    CheckActor(slot, move.actor_object_id);
    for (auto coordinate : move.position) {
      if (!std::isfinite(coordinate)) {
        throw std::runtime_error("Non-finite room movement.");
      }
    }
    // Remote actors exist only after loading. Never queue old positions into a
    // new map.
    if (phase_ != RoomGamePhase::kPlaying) {
      return {};
    }
    if (member.actor_phase != RoomActorPhase::kAlive &&
        member.actor_phase != RoomActorPhase::kBubble) {
      return {};
    }
    member.movement.Record(now, move.position);
    return {{move.Encode(), {}, slot}};
  }
  if (packet.message_id == GameHitNotification::kMessageId ||
      packet.message_id == GameDamagerRemoveNotification::kMessageId) {
    return ProcessHit(slot, packet);
  }
  if (packet.message_id == GameActorDamageResultNotification::kMessageId) {
    return ProcessDamageResult(slot, packet);
  }
  if (packet.message_id == GameBubbleSettledNotification::kMessageId) {
    const auto settled = GameBubbleSettledNotification::Decode(packet);
    CheckActor(slot, settled.actor_object_id);
    if (Ended() || phase_ != RoomGamePhase::kPlaying ||
        member.actor_phase != RoomActorPhase::kBubble ||
        member.bubble_settled) {
      return {};
    }
    // Echo to the owner too: native +552 requests state 301. Merely
    // recording Bubble on the server leaves every client rolling.
    std::vector<RoomGameDelivery> output{{packet, {}, {}}};
    member.bubble_settled = true;
    return output;
  }
  if (packet.message_id == GameWeaponSwitchNotification::kMessageId) {
    const auto change = GameWeaponSwitchNotification::Decode(packet);
    CheckActor(slot, change.actor_object_id);
    if (change.weapon_index >= member.weapons.size() ||
        change.select_value > 1 || change.value_at20 > 1) {
      throw std::runtime_error("Invalid ordinary weapon switch fields.");
    }
    if (Ended() || member.actor_phase != RoomActorPhase::kAlive ||
        change.weapon_id == 0 ||
        member.weapons[change.weapon_index] != change.weapon_id) {
      return {};
    }
    return {{packet, {}, {}}};
  }
  if (packet.message_id == GameBubbleActionStartNotification::kMessageId ||
      packet.message_id == GameBubbleActionEndNotification::kMessageId ||
      packet.message_id == GameBubbleActionCompleteNotification::kMessageId) {
    return ProcessBubbleAction(slot, packet);
  }
  if (packet.message_id == GameMeleeAttackNotification::kMessageId) {
    return ProcessMeleeAttack(slot, packet);
  }
  if (packet.message_id == GameWeaponImpactNotification::kMessageId) {
    return ProcessWeaponImpact(slot, packet);
  }
  if (packet.message_id == GameGrenadeDetonateNotification::kMessageId) {
    return ProcessGrenadeDetonate(slot, packet);
  }
  if (packet.message_id == GameStompStartNotification::kMessageId ||
      packet.message_id == GameStompHitNotification::kMessageId) {
    return ProcessStomp(slot, packet);
  }
  if (packet.message_id == ClientToServerMessage0x800B::kMessageId ||
      packet.message_id == ClientToServerMessage0x800C::kMessageId ||
      packet.message_id == ClientToServerMessage0x800D::kMessageId ||
      packet.message_id == ClientToServerMessage0x800E::kMessageId ||
      packet.message_id == ClientToServerMessage0x802D::kMessageId) {
    return ProcessActorAction(slot, packet);
  }
  if (packet.message_id == GameBubbleExitNotification::kMessageId ||
      packet.message_id == GameBubbleShotPopRequest::kMessageId ||
      packet.message_id == GameBubbleContactPopRequest::kMessageId ||
      packet.message_id == GameBubbleDeflectNotification::kMessageId ||
      packet.message_id == GameBubblePopNotification::kMessageId ||
      packet.message_id == GameBubbleRescueRequest::kMessageId ||
      packet.message_id == GameBubbleRescueNotification::kMessageId ||
      packet.message_id == GameBubbleRescueEffectNotification::kMessageId) {
    return ProcessBubble(slot, packet);
  }
  // Ordinary guns and equipped grenades share the GoDamager key range.
  if (packet.message_id == GameShotNotification::kMessageId ||
      packet.message_id == GameExtendedShotNotification::kMessageId) {
    const auto extended =
        packet.message_id == GameExtendedShotNotification::kMessageId
            ? GameExtendedShotNotification::Decode(packet)
            : GameExtendedShotNotification{};
    const auto shot = packet.message_id == GameShotNotification::kMessageId
                          ? GameShotNotification::Decode(packet).shot
                          : extended.shot;
    CheckActor(slot, shot.actor_object_id);
    // The explicit geometry checks are shared with the existing codec's field
    // semantics.
    if (!shot.HasFiniteCoordinates() || shot.weapon_index >= 6) {
      throw std::runtime_error(
          "Unsupported room shot geometry or weapon index.");
    }
    if (Ended()) {
      return {};
    }
    if (phase_ != RoomGamePhase::kPlaying) {
      throw std::runtime_error("Shot before shared game start.");
    }
    // In-flight shots during this player's death cannot create new
    // damage objects. Other participants retain their own live state.
    if (member.actor_phase != RoomActorPhase::kAlive) {
      return {};
    }
    const auto* weapon =
        battle::WeaponCatalog::Find(member.weapons[shot.weapon_index]);
    const bool ordinary_gun =
        weapon && (weapon->master_id == 1000 || weapon->master_id == 2000 ||
                   weapon->master_id == 3000 || weapon->master_id == 4000);
    // E092 verifies all four master families against current resources:
    // their category is unchanged and isExplosive is false. Alternate
    // resources and reinforcement lists need a separate derivation.
    const bool grenade =
        weapon && weapon->master_id == 5000 && shot.weapon_index == 3;
    const bool tracked = (ordinary_gun || grenade) &&
                         extended.value_at40 == 0 &&
                         extended.values_at44.empty();
    const auto count = tracked && weapon->is_spread ? weapon->spread_count : 1U;
    const auto first = slot * 100000U;
    if (count == 0 || count > 100000U || shot.shot_sequence < first ||
        shot.shot_sequence >= first + 100000U ||
        count > first + 100000U - shot.shot_sequence) {
      throw std::runtime_error(
          "Shot damager keys escape the authenticated room slot.");
    }
    // Gaps are legal: other native producers consume this same counter.
    // The per-match high-water mark prevents replay after a key retires.
    if (shot.shot_sequence < member.next_damager_key) {
      return {};
    }
    if (scene_) {
      std::erase_if(member.damagers, [&](const auto& entry) {
        return now - entry.second.created > kDamageResultLifetime;
      });
    }
    // Explicit local memory bound, not a recovered native ammo limit.
    if (tracked && member.damagers.size() + count > 4096) {
      throw std::runtime_error("Too many active room damage objects.");
    }
    // The original thrower already consumes its shot locally (origin 1).
    // Echoing a grenade creates a second body with the same damager key.
    std::vector<RoomGameDelivery> deliveries{
        {packet, {}, grenade ? std::optional{slot} : std::nullopt}};
    if (tracked) {
      const auto origin = battle::CombatGeometry::DecodeBinary16(shot.position);
      const auto source = member.movement.Latest(now, std::chrono::seconds{5});
      std::array<std::uint64_t, 16> epochs{};
      for (const auto& [targetSlot, targetMember] : members_) {
        epochs[targetSlot] = targetMember.movement_epoch;
      }
      for (std::uint32_t i = 0; i < count; ++i) {
        member.damagers.emplace(
            shot.shot_sequence + i,
            Damager{weapon->weapon_id, 0, origin, now, source, epochs,
                    grenade ? DamagerKind::kGrenade : DamagerKind::kBullet});
      }
    }
    member.next_damager_key = shot.shot_sequence + count;
    return deliveries;
  }
  return ProcessActor(slot, packet);
}

}  // namespace bubble_fighter::server::features::rooms
