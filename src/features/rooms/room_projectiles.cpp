#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "features/battle/combat_geometry.h"
#include "features/battle/combat_scene.h"
#include "features/battle/combat_weapon_catalog.h"
#include "features/battle/weapon_catalog.h"
#include "features/rooms/room_game_session.h"
#include "protocol/battle/game_actor_damage_result_notification.h"
#include "protocol/battle/game_damager_remove_notification.h"
#include "protocol/battle/game_grenade_detonate_notification.h"
#include "protocol/battle/game_hit_notification.h"
#include "protocol/battle/game_melee_attack_notification.h"
#include "protocol/battle/game_stomp_hit_notification.h"
#include "protocol/battle/game_stomp_start_notification.h"
#include "protocol/battle/game_weapon_impact_notification.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/unresolved/client_to_server_message_0x802d.h"

namespace bubble_fighter::server::features::rooms {
std::vector<RoomGameDelivery> RoomGameSession::ProcessHit(
    std::uint32_t slot, const protocol::TcpPacket& packet) {
  using namespace protocol;
  const bool is_hit = packet.message_id == GameHitNotification::kMessageId;
  const auto hit =
      is_hit ? GameHitNotification::Decode(packet) : GameHitNotification{};
  const auto key =
      is_hit ? hit.damager_key
             : GameDamagerRemoveNotification::Decode(packet).damager_key;
  if (key < slot * 100000U || key >= (slot + 1U) * 100000U) {
    throw std::runtime_error(
        "Damage object does not belong to the authenticated room slot.");
  }
  if (is_hit) {
    for (auto coordinate : hit.position) {
      if ((coordinate & 0x7C00U) == 0x7C00U) {
        throw std::runtime_error("Non-finite room hit position.");
      }
    }
  }
  if (Ended()) {
    return {};
  }
  if (phase_ != RoomGamePhase::kPlaying) {
    throw std::runtime_error("Hit before shared game start.");
  }
  auto& member = members_.at(slot);
  auto found = member.damagers.find(key);
  // Missing, retired, unsupported, and repeated reports cannot create a
  // damage object or revive a retired key. Native missing lookup is inert.
  if (found == member.damagers.end()) {
    return {};
  }
  if (!is_hit) {
    std::vector<RoomGameDelivery> deliveries{{packet, {}, {}}};
    member.damagers.erase(found);
    return deliveries;
  }
  if (found->second.kind != DamagerKind::kBullet) {
    return {};
  }
  std::optional<std::uint32_t> target;
  for (const auto& [candidate, state] : members_) {
    (void)state;
    if (hit.target_object_id == ActorId(candidate)) {
      target = candidate;
      break;
    }
  }
  // Map/NPC identity has not been established by this room. Retire an
  // owned non-retained projectile without forwarding an unknown target.
  if (!target) {
    if (hit.keep_damager_value != 0) {
      return {};
    }
    std::vector<RoomGameDelivery> deliveries{
        {GameDamagerRemoveNotification{key}.Encode(), {}, {}}};
    member.damagers.erase(found);
    return deliveries;
  }
  const auto bit = static_cast<std::uint16_t>(1U << *target);
  // A pre-transition report must not be applied to a revived actor at
  // the same object ID, even if fresh coordinates have already arrived.
  if (found->second.target_movement_epochs[*target] !=
      members_.at(*target).movement_epoch) {
    return {};
  }
  if (scene_) {
    const auto& damager = found->second;
    const auto target_position = members_.at(*target).movement.Latest(
        last_observed_, std::chrono::seconds{5});
    // Local broad consistency envelope, NOT the native bone hitboxes.
    // Receipt timestamps cannot be rewound by the client's packed time.
    constexpr double report_envelope_squared = 64.0 * 64.0;
    const auto point = battle::CombatGeometry::DecodeBinary16(hit.position);
    if (last_observed_ - damager.created > kDamageResultLifetime ||
        !damager.reported_source_position || !target_position ||
        battle::CombatGeometry::DistanceSquared(
            damager.origin, *damager.reported_source_position) >
            report_envelope_squared ||
        battle::CombatGeometry::DistanceSquared(point, *target_position) >
            report_envelope_squared) {
      return {};
    }
    const auto obstruction = scene_->FirstObstruction(
        damager.origin, point, battle::CombatSceneQuery::kBullet);
    // Half-float contact coordinates are quantized. Exclude only walls
    // at least two world units before the reported contact point.
    const auto distance = std::sqrt(
        battle::CombatGeometry::DistanceSquared(damager.origin, point));
    if (obstruction && (1.0 - *obstruction) * distance > 2.0) {
      return {};
    }
  }
  if ((found->second.hit_slots & bit) != 0) {
    if (hit.keep_damager_value != 0) {
      return {};
    }
    std::vector<RoomGameDelivery> deliveries{
        {GameDamagerRemoveNotification{key}.Encode(), {}, {}}};
    member.damagers.erase(found);
    return deliveries;
  }
  std::vector<RoomGameDelivery> deliveries{{packet, {}, {}}};
  auto& victim = members_.at(*target);
  if (victim.actor_phase == RoomActorPhase::kAlive) {
    victim.damage_sources.insert_or_assign(
        {ActorId(slot), found->second.weapon_id},
        last_observed_ + kDamageResultLifetime);
  }
  if (hit.keep_damager_value == 0) {
    member.damagers.erase(found);
  } else {
    found->second.hit_slots |= bit;
  }
  // The original clients apply their weapon/target logic. This relay
  // does not infer HP, confirm physical collision, score, or grant rewards.
  return deliveries;
}

std::vector<RoomGameDelivery> RoomGameSession::ProcessDamageResult(
    std::uint32_t slot, const protocol::TcpPacket& packet) {
  const auto report =
      protocol::GameActorDamageResultNotification::Decode(packet);
  CheckActor(slot, report.actor_object_id);
  std::optional<std::uint32_t> source_slot;
  for (const auto& [candidate, state] : members_) {
    (void)state;
    if (report.source_object_id == ActorId(candidate)) {
      source_slot = candidate;
    }
  }
  if (!source_slot) {
    throw std::runtime_error("Room damage source is not a participant.");
  }
  for (auto bits : report.vector_bits) {
    if ((bits & 0x7F800000U) == 0x7F800000U) {
      throw std::runtime_error("Non-finite room damage result vector.");
    }
  }
  // 6E3B80 emits byte-sized flags as uint32 and a zero final field.
  // Preserve the complete time word, including its unspecified high byte.
  if (report.direct_death_value > 1 || report.full_health_hit_value > 1 ||
      report.value_at36 != 0) {
    throw std::runtime_error("Unsupported ordinary-room damage result flags.");
  }
  if (Ended()) {
    return {};
  }
  if (phase_ != RoomGamePhase::kPlaying) {
    throw std::runtime_error("Damage result before shared game start.");
  }
  auto& member = members_.at(slot);
  if (member.actor_phase != RoomActorPhase::kAlive) {
    return {};
  }
  // A result cannot authorize an unsolicited victim, weapon, or source.
  // Keep unmatched reports inert, just like an absent native hit object.
  const auto pending =
      member.damage_sources.find({report.source_object_id, report.weapon_id});
  if (pending == member.damage_sources.end()) {
    return {};
  }
  if (pending->second <= last_observed_) {
    member.damage_sources.erase(pending);
    return {};
  }
  std::vector<RoomGameDelivery> deliveries{{packet, {}, {}}};
  const bool enemies = member.team != 0 &&
                       members_.at(*source_slot).team != 0 &&
                       member.team != members_.at(*source_slot).team;
  if (enemies) {
    AppendScore(
        deliveries,
        report.direct_death_value ? ScoreEvent::kPang : ScoreEvent::kBubble,
        *source_slot, slot);
  } else if (report.direct_death_value) {
    AppendScore(deliveries, ScoreEvent::kDeath, slot, slot);
  }
  member.actor_phase = report.direct_death_value ? RoomActorPhase::kDown
                                                 : RoomActorPhase::kBubble;
  ResetMovement(member);
  ClearBubbleLinks(slot);
  member.respawn_completed = false;
  member.damage_sources.clear();
  return deliveries;
}

std::vector<RoomGameDelivery> RoomGameSession::ProcessMeleeAttack(
    std::uint32_t slot, const protocol::TcpPacket& packet) {
  const auto attack = protocol::GameMeleeAttackNotification::Decode(packet);
  CheckActor(slot, attack.actor_object_id);
  const auto first = slot * 100000U;
  if (attack.shot_sequence < first || attack.shot_sequence >= first + 100000U) {
    throw std::runtime_error(
        "Melee damage key escapes the authenticated room slot.");
  }
  auto& member = members_.at(slot);
  if (Ended() || phase_ != RoomGamePhase::kPlaying ||
      member.actor_phase != RoomActorPhase::kAlive) {
    return {};
  }
  const auto* weapon = battle::WeaponCatalog::Find(attack.weapon_id);
  // The ordinary equipped melee family is proven by the current
  // resource and 8ECB00/6F2700. Other forms and modifier lists need
  // their own authority; a client cannot supply a different loadout.
  if (!weapon || weapon->master_id != 7000 ||
      member.weapons[2] != attack.weapon_id || !attack.values.empty()) {
    return {};
  }
  if (attack.shot_sequence < member.next_damager_key) {
    return {};
  }
  std::erase_if(member.damagers, [&](const auto& entry) {
    return last_observed_ - entry.second.created > kDamageResultLifetime;
  });
  if (member.damagers.size() >= 4096) {
    throw std::runtime_error("Too many active room damage objects.");
  }
  const auto source =
      member.movement.Latest(last_observed_, std::chrono::seconds{5});
  std::array<std::uint64_t, 16> epochs{};
  for (const auto& [targetSlot, targetMember] : members_) {
    epochs[targetSlot] = targetMember.movement_epoch;
  }
  // 6F2700 supplies the wire sequence through 7A1270 -> 795BB0
  // to GoDamager+692, shared with 8215 impact/808B removal. Coordinates are
  // the last reported actor position, not new client authority.
  member.damagers.emplace(
      attack.shot_sequence,
      Damager{attack.weapon_id, 0, source.value_or(battle::CombatVector{}),
              last_observed_, source, epochs, DamagerKind::kMelee});
  member.next_damager_key = attack.shot_sequence + 1;
  // +432 invokes the original attack animation on both local and
  // remote actors; the client also constructs its native melee object.
  return {{packet, {}, {}}};
}

std::vector<RoomGameDelivery> RoomGameSession::ProcessWeaponImpact(
    std::uint32_t slot, const protocol::TcpPacket& packet) {
  const auto impact = protocol::GameWeaponImpactNotification::Decode(packet);
  CheckActor(slot, impact.source_object_id);
  if (impact.damager_key < slot * 100000U ||
      impact.damager_key >= (slot + 1U) * 100000U) {
    throw std::runtime_error(
        "Weapon impact key escapes the authenticated room slot.");
  }
  for (const auto bits : impact.position_bits) {
    if (!std::isfinite(std::bit_cast<float>(bits))) {
      throw std::runtime_error("Non-finite weapon impact.");
    }
  }
  if (Ended() || phase_ != RoomGamePhase::kPlaying) {
    return {};
  }
  auto& member = members_.at(slot);
  const auto found = member.damagers.find(impact.damager_key);
  if (found == member.damagers.end()) {
    return {};
  }
  auto& damager = found->second;
  if ((damager.kind != DamagerKind::kMelee &&
       damager.kind != DamagerKind::kScatter) ||
      last_observed_ - damager.created > kDamageResultLifetime) {
    return {};
  }
  std::optional<std::uint32_t> target;
  for (const auto& [candidate, state] : members_) {
    (void)state;
    if (ActorId(candidate) == impact.target_object_id) {
      target = candidate;
      break;
    }
  }
  if (!target) {
    return {};
  }
  auto& victim = members_.at(*target);
  const auto bit = static_cast<std::uint16_t>(1U << *target);
  if ((damager.hit_slots & bit) ||
      damager.target_movement_epochs[*target] != victim.movement_epoch) {
    return {};
  }
  const battle::CombatVector point{
      std::bit_cast<float>(impact.position_bits[0]),
      std::bit_cast<float>(impact.position_bits[1]),
      std::bit_cast<float>(impact.position_bits[2])};
  if (scene_) {
    const auto position =
        victim.movement.Latest(last_observed_, std::chrono::seconds{5});
    // Same local report envelope as ordinary shots; not native bones or
    // authoritative HP.
    constexpr double envelope = 64.0 * 64.0;
    if (!position || !damager.reported_source_position ||
        battle::CombatGeometry::DistanceSquared(point, *position) > envelope ||
        battle::CombatGeometry::DistanceSquared(
            damager.origin, *damager.reported_source_position) > envelope) {
      return {};
    }
    const auto obstruction = scene_->FirstObstruction(
        damager.origin, point, battle::CombatSceneQuery::kBullet);
    const auto distance = std::sqrt(
        battle::CombatGeometry::DistanceSquared(damager.origin, point));
    if (obstruction && (1.0 - *obstruction) * distance > 2.0) {
      return {};
    }
  }
  if (victim.actor_phase == RoomActorPhase::kAlive) {
    victim.damage_sources.insert_or_assign(
        {ActorId(slot), damager.weapon_id},
        last_observed_ + kDamageResultLifetime);
  }
  damager.hit_slots |= bit;
  // 6F4EF0 applies native melee/scatter damage. 808B independently retires the
  // object.
  return {{packet, {}, {}}};
}

std::vector<RoomGameDelivery> RoomGameSession::ProcessGrenadeDetonate(
    std::uint32_t slot, const protocol::TcpPacket& packet) {
  const auto data = protocol::GameGrenadeDetonateNotification::Decode(packet);
  for (const auto key : {data.source_damager_key, data.damager_key}) {
    if (key < slot * 100000U || key >= (slot + 1U) * 100000U) {
      throw std::runtime_error(
          "Grenade key escapes the authenticated room slot.");
    }
  }
  for (const auto& vector :
       {std::span<const std::uint32_t>(data.position_bits),
        std::span<const std::uint32_t>(data.vector_bits)}) {
    for (const auto bits : vector) {
      if (!std::isfinite(std::bit_cast<float>(bits))) {
        throw std::runtime_error("Non-finite grenade geometry.");
      }
    }
  }
  if (Ended() || phase_ != RoomGamePhase::kPlaying || !data.values.empty()) {
    return {};
  }
  auto& member = members_.at(slot);
  const auto source = member.damagers.find(data.source_damager_key);
  if (source == member.damagers.end() ||
      source->second.kind != DamagerKind::kGrenade ||
      data.damager_key < member.next_damager_key ||
      last_observed_ - source->second.created > kDamageResultLifetime) {
    return {};
  }
  const battle::CombatVector point{std::bit_cast<float>(data.position_bits[0]),
                                   std::bit_cast<float>(data.position_bits[1]),
                                   std::bit_cast<float>(data.position_bits[2])};
  if (scene_) {
    const auto* weapon =
        battle::CombatWeaponCatalog::Find(source->second.weapon_id);
    if (!weapon || !source->second.reported_source_position) {
      return {};
    }
    // Weapon Speed is an engine input, not recovered world units/second.
    // Original 5033 travels about 305 units in three seconds with Speed=11.
    // Keep source consistency; trajectory/bounces remain client simulated.
    if (battle::CombatGeometry::DistanceSquared(
            source->second.origin, *source->second.reported_source_position) >
        64.0 * 64.0) {
      return {};
    }
  }
  const Damager scatter{
      source->second.weapon_id, 0,     point,
      last_observed_,           point, source->second.target_movement_epochs,
      DamagerKind::kScatter};
  member.damagers.emplace(data.damager_key, scatter);
  member.damagers.erase(source);
  member.next_damager_key = data.damager_key + 1;
  // 6DF570 constructs GoScatter and destroys the parent. The thrower's
  // local consume performs the same operation; deliver once to its peers.
  return {{packet, {}, slot}};
}

std::vector<RoomGameDelivery> RoomGameSession::ProcessStomp(
    std::uint32_t slot, const protocol::TcpPacket& packet) {
  using namespace protocol;
  const bool start =
      packet.message_id == GameStompStartNotification::kMessageId;
  const auto hit = start ? GameStompHitNotification{}
                         : GameStompHitNotification::Decode(packet);
  const auto data = start ? GameStompStartNotification::Decode(packet)
                          : GameStompStartNotification{hit.actor_object_id,
                                                       hit.target_object_id};
  CheckActor(slot, data.actor_object_id);
  auto& member = members_.at(slot);
  if (Ended() || phase_ != RoomGamePhase::kPlaying ||
      member.actor_phase != RoomActorPhase::kAlive) {
    return {};
  }
  std::optional<std::uint32_t> target;
  for (const auto& [candidate, state] : members_) {
    (void)state;
    if (ActorId(candidate) == data.target_object_id) {
      target = candidate;
      break;
    }
  }
  if (!target || *target == slot) {
    return {};
  }
  auto& victim = members_.at(*target);
  // Both 492E70 and 4A8BA0 require native state 401, established by
  // 8044 (respawn wait). A living bubble is state 301 and uses 8017.
  const bool eligible = victim.actor_phase == RoomActorPhase::kWaiting &&
                        member.team && victim.team &&
                        member.team != victim.team;
  if (start) {
    if (!eligible || (member.stomp && member.stomp->expires > last_observed_)) {
      return {};
    }
    member.stomp =
        Member::StompAction{*target, victim.movement_epoch,
                            last_observed_ + std::chrono::seconds{5}, false};
  } else {
    if (!member.stomp || member.stomp->target_slot != *target ||
        member.stomp->hit) {
      return {};
    }
    if (!eligible || member.stomp->target_epoch != victim.movement_epoch ||
        member.stomp->expires <= last_observed_) {
      // Local cancellation policy: finish the actor, never hit a new life with
      // an old action.
      member.stomp.reset();
      return {{ClientToServerMessage0x802D{ActorId(slot)}.Encode(), {}, {}}};
    }
    member.stomp->hit = true;
  }
  // Native 8030 animates the attacker/target and consumes their stomp effects;
  // it does not prove a pang. Do not invent 9585, deaths, or score here.
  return {{packet, {}, {}}};
}
}  // namespace bubble_fighter::server::features::rooms
