#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "features/rooms/room_game_session.h"
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
#include "protocol/battle/game_bubble_shot_pop_request.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::features::rooms {
std::vector<RoomGameDelivery> RoomGameSession::ProcessBubbleAction(
    std::uint32_t slot, const protocol::TcpPacket& packet) {
  using namespace protocol;
  const bool start =
      packet.message_id == GameBubbleActionStartNotification::kMessageId;
  const bool complete =
      packet.message_id == GameBubbleActionCompleteNotification::kMessageId;
  const auto action =
      start
          ? GameBubbleActionStartNotification::Decode(packet)
          : GameBubbleActionStartNotification{
                complete ? GameBubbleActionCompleteNotification::Decode(packet)
                               .actor_object_id
                         : GameBubbleActionEndNotification::Decode(packet)
                               .actor_object_id,
                0, 0};
  CheckActor(slot, action.actor_object_id);
  auto& member = members_.at(slot);
  if (Ended() || phase_ != RoomGamePhase::kPlaying ||
      member.actor_phase != RoomActorPhase::kAlive) {
    return {};
  }
  if (!start) {
    if (!member.action) {
      return {};
    }
    if (!complete && member.action->ended) {
      return {};
    }
    std::vector<RoomGameDelivery> output{{packet, {}, {}}};
    if (complete) {
      member.action.reset();
    } else {
      member.action->ended = true;
    }
    return output;
  }
  // Ordinary 50AEA0 and 8F2500 producers. NPC/item variants require
  // their own authority; do not turn them into a room actor.
  if (action.variant != 1 && action.variant != 2) {
    return {};
  }
  std::optional<std::uint32_t> target;
  for (const auto& [candidate, value] : members_) {
    if (ActorId(candidate) == action.target_object_id &&
        value.actor_phase == RoomActorPhase::kBubble && member.team != 0 &&
        value.team != 0 && value.team != member.team) {
      target = candidate;
      break;
    }
  }
  if (!target) {
    return {};
  }
  const auto epoch = members_.at(*target).movement_epoch;
  if (member.action && !member.action->ended &&
      member.action->expires > last_observed_ &&
      member.action->target_slot == *target &&
      member.action->variant == action.variant &&
      member.action->target_epochs[*target] == epoch) {
    return {};
  }
  Member::BubbleAction pending{*target, action.variant,
                               last_observed_ + std::chrono::seconds{5}};
  for (const auto& [candidate, value] : members_) {
    if (value.actor_phase == RoomActorPhase::kBubble && value.team != 0 &&
        value.team != member.team) {
      pending.eligible_targets |= static_cast<std::uint16_t>(1U << candidate);
      pending.target_epochs[candidate] = value.movement_epoch;
    }
  }
  // Owner also needs the echo: +508 clears its pending flag and enters
  // state 110. Distance/animation collision is still client-reported.
  std::vector<RoomGameDelivery> output{{packet, {}, {}}};
  member.action = pending;
  return output;
}

std::vector<RoomGameDelivery> RoomGameSession::ProcessBubble(
    std::uint32_t slot, const protocol::TcpPacket& packet) {
  using namespace protocol;
  const auto id = packet.message_id;
  std::uint32_t actor = 0, source = 0, contact = 0, reason = 0;
  std::uint8_t kind = 0;
  const auto finite = [](std::uint32_t bits) {
    if ((bits & 0x7F800000U) == 0x7F800000U) {
      throw std::runtime_error("Non-finite bubble result value.");
    }
  };
  if (id == GameBubbleExitNotification::kMessageId) {
    const auto data = GameBubbleExitNotification::Decode(packet);
    actor = data.actor_object_id;
    source = data.source_object_id;
    CheckActor(slot, actor);
    // 510300 and 85C4C0 use these two sentinel sources.
    if (source != 0xFFFFFFFFU && source != 0xFFFFFFFEU) {
      throw std::runtime_error("Unsupported bubble exit source.");
    }
  } else if (id == GameBubbleShotPopRequest::kMessageId) {
    const auto data = GameBubbleShotPopRequest::Decode(packet);
    actor = data.actor_object_id;
    source = data.source_object_id;
    // Normal action 110 (8F0EF0) is sent by the source. The older
    // victim-owned 485BE0/weapon-5300 path remains unsupported.
    if (source != ActorId(slot)) {
      CheckActor(slot, actor);
      return {};
    }
    CheckActor(slot, source);
  } else if (id == GameBubbleContactPopRequest::kMessageId) {
    const auto data = GameBubbleContactPopRequest::Decode(packet);
    actor = data.actor_object_id;
    source = data.source_object_id;
    contact = 1;
    CheckActor(slot, source);
  } else if (id == GameBubbleDeflectNotification::kMessageId) {
    const auto data = GameBubbleDeflectNotification::Decode(packet);
    actor = data.actor_object_id;
    source = data.source_object_id;
    contact = data.contact_value;
    for (auto bits : data.vector_bits) {
      finite(bits);
    }
    if (data.value_at20 != 0xFFFFFFFFU || contact > 1) {
      throw std::runtime_error("Unsupported ordinary-room bubble deflection.");
    }
    CheckActor(slot, source);
  } else if (id == GameBubblePopNotification::kMessageId) {
    const auto data = GameBubblePopNotification::Decode(packet);
    actor = data.actor_object_id;
    source = data.source_object_id;
    contact = data.contact_value;
    reason = data.reason_value;
    for (auto bits : data.vector_bits) {
      finite(bits);
    }
    if (data.apply_value != 1 || contact > 1 || (reason != 0 && reason != 2)) {
      throw std::runtime_error("Unsupported ordinary-room bubble pop.");
    }
    CheckActor(slot, source);
  } else if (id == GameBubbleRescueRequest::kMessageId) {
    const auto data = GameBubbleRescueRequest::Decode(packet);
    actor = data.actor_object_id;
    source = data.helper_object_id;
    kind = data.kind;
    // 50A4D0 contact: helper reports; 485BE0 gun hit: victim reports.
    if (kind != 0 && kind != 1 && kind != 3) {
      throw std::runtime_error("Unsupported ordinary-room rescue kind.");
    }
    CheckActor(slot, kind == 3 ? actor : source);
  } else if (id == GameBubbleRescueNotification::kMessageId) {
    const auto data = GameBubbleRescueNotification::Decode(packet);
    actor = data.actor_object_id;
    source = data.helper_object_id;
    kind = data.kind;
    finite(data.value_at8);
    CheckActor(slot, actor);
  } else {
    const auto data = GameBubbleRescueEffectNotification::Decode(packet);
    actor = data.actor_object_id;
    source = data.helper_object_id;
    kind = data.kind;
    // 4AAE70 writes the helper first, but the local victim sends it.
    CheckActor(slot, actor);
  }
  const auto find_slot = [&](std::uint32_t object) {
    for (const auto& [candidate, member] : members_) {
      (void)member;
      if (ActorId(candidate) == object) {
        return candidate;
      }
    }
    throw std::runtime_error("Bubble actor is not a room participant.");
  };
  const auto target_slot = find_slot(actor);
  const auto source_slot = id == GameBubbleExitNotification::kMessageId
                               ? target_slot
                               : find_slot(source);
  if (Ended()) {
    return {};
  }
  if (phase_ != RoomGamePhase::kPlaying) {
    throw std::runtime_error("Bubble interaction before shared game start.");
  }
  // Gun rescue kind 3 still requires weapon-5300 authority. 957F also
  // has a normal action producer, independently verified in 8F0EF0.
  if (id == GameBubbleRescueRequest::kMessageId && kind == 3) {
    return {};
  }
  auto& target = members_.at(target_slot);
  auto& helper = members_.at(source_slot);
  // Explicit local pending lifetime; not the original rescue timer.
  const auto expires = last_observed_ + std::chrono::seconds{5};
  for (auto* pending : {&target.pop, &target.rescue, &target.rescue_effect}) {
    if (*pending && (*pending)->expires <= last_observed_) {
      pending->reset();
    }
  }
  const auto matches = [&](const std::optional<Member::Interaction>& pending,
                           std::uint8_t value) {
    return pending && pending->source_slot == source_slot &&
           pending->kind == value;
  };
  const bool same_team =
      target.team != 0 && helper.team != 0 && target.team == helper.team;
  const bool enemies =
      target.team != 0 && helper.team != 0 && target.team != helper.team;
  const auto forward = [&]() {
    return std::vector<RoomGameDelivery>{{packet, {}, {}}};
  };
  if (id == GameBubbleRescueEffectNotification::kMessageId) {
    if (target.actor_phase != RoomActorPhase::kAlive || !same_team ||
        !matches(target.rescue_effect, kind)) {
      return {};
    }
    auto deliveries = forward();
    target.rescue_effect.reset();
    return deliveries;
  }
  if (target.actor_phase != RoomActorPhase::kBubble) {
    return {};
  }
  if (id == GameBubbleExitNotification::kMessageId) {
    auto deliveries = forward();
    ClearBubbleLinks(target_slot);
    target.actor_phase = RoomActorPhase::kAlive;
    ResetMovement(target);
    target.damage_sources.clear();
    return deliveries;
  }
  if (id == GameBubbleRescueRequest::kMessageId) {
    if (source_slot == target_slot || !same_team ||
        helper.actor_phase != RoomActorPhase::kAlive || target.pop ||
        target.rescue) {
      return {};
    }
    auto deliveries = forward();
    target.rescue = Member::Interaction{source_slot, kind, expires};
    return deliveries;
  }
  if (id == GameBubbleRescueNotification::kMessageId) {
    if (!same_team || helper.actor_phase != RoomActorPhase::kAlive ||
        !matches(target.rescue, kind)) {
      return {};
    }
    auto deliveries = forward();
    AppendScore(deliveries, ScoreEvent::kRescue, source_slot, target_slot);
    ClearBubbleLinks(target_slot);
    target.actor_phase = RoomActorPhase::kAlive;
    ResetMovement(target);
    target.damage_sources.clear();
    target.rescue_effect = Member::Interaction{source_slot, kind, expires};
    return deliveries;
  }
  if (id == GameBubbleContactPopRequest::kMessageId ||
      id == GameBubbleShotPopRequest::kMessageId) {
    if (!enemies || target.pop || target.rescue ||
        helper.actor_phase != RoomActorPhase::kAlive) {
      return {};
    }
    const auto mask = static_cast<std::uint16_t>(1U << target_slot);
    if (id == GameBubbleShotPopRequest::kMessageId &&
        (!helper.action || helper.action->ended ||
         helper.action->expires <= last_observed_ ||
         !(helper.action->eligible_targets & mask) ||
         (helper.action->requested_targets & mask) ||
         helper.action->target_epochs[target_slot] != target.movement_epoch)) {
      return {};
    }
    auto deliveries = forward();
    target.pop = Member::Interaction{
        source_slot, static_cast<std::uint8_t>(contact), expires};
    if (id == GameBubbleShotPopRequest::kMessageId) {
      helper.action->requested_targets |= mask;
    }
    return deliveries;
  }
  // 85C4C0 can report an own timed pop (contact=1, reason=2).
  // Other area/item producers require evidence beyond this ordinary path.
  const bool self_pop = id == GameBubblePopNotification::kMessageId &&
                        target_slot == source_slot && contact == 1 &&
                        reason == 2;
  if (!self_pop && (!enemies || reason != 0 ||
                    !matches(target.pop, static_cast<std::uint8_t>(contact)))) {
    return {};
  }
  auto deliveries = forward();
  if (id == GameBubbleDeflectNotification::kMessageId) {
    target.pop.reset();  // 6FB680 applies effects, leaving the bubble.
  } else {
    AppendScore(deliveries, self_pop ? ScoreEvent::kDeath : ScoreEvent::kPang,
                self_pop ? target_slot : source_slot, target_slot);
    ClearBubbleLinks(target_slot);
    target.actor_phase =
        RoomActorPhase::kDown;  // native 4AEA80 requests state 405.
    ResetMovement(target);
    target.respawn_completed = false;
    target.damage_sources.clear();
  }
  return deliveries;
}
}  // namespace bubble_fighter::server::features::rooms
