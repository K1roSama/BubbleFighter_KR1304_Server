#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "features/rooms/room_game_session.h"
#include "protocol/battle/game_actor_death_notification.h"
#include "protocol/battle/game_actor_respawn_wait_notification.h"
#include "protocol/battle/game_respawn_completion_notification.h"
#include "protocol/battle/game_respawn_notification.h"
#include "protocol/battle/game_respawn_request.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/unresolved/client_to_server_message_0x800b.h"
#include "protocol/unresolved/client_to_server_message_0x800c.h"
#include "protocol/unresolved/client_to_server_message_0x800d.h"
#include "protocol/unresolved/client_to_server_message_0x800e.h"
#include "protocol/unresolved/client_to_server_message_0x802d.h"

namespace bubble_fighter::server::features::rooms {
std::vector<RoomGameDelivery> RoomGameSession::ProcessActor(
    std::uint32_t slot, const protocol::TcpPacket& packet) {
  using namespace protocol;
  std::uint32_t actor = 0;
  TcpPacket response;
  if (packet.message_id == GameActorDeathNotification::kMessageId) {
    actor = GameActorDeathNotification::Decode(packet).actor_object_id;
    response = packet;
  } else if (packet.message_id ==
             GameActorRespawnWaitNotification::kMessageId) {
    actor = GameActorRespawnWaitNotification::Decode(packet).actor_object_id;
    response = packet;
  } else if (packet.message_id == GameRespawnRequest::kMessageId) {
    const auto request = GameRespawnRequest::Decode(packet);
    actor = request.parameters.actor_object_id;
    // E091: mode 1 reaches 515C55 with absent optional parameters.
    // The alternative selector at 9EC0A0 requires mode 12. The current
    // resource's ordinary itemResetOnRespawn is zero (6E3730).
    if (request.parameters.value_at4 || request.parameters.value_at12 ||
        request.reset_items_value != 0) {
      throw std::runtime_error("Unsupported ordinary-room respawn parameters.");
    }
    // Explicit unrewarded local policy: preserve the member +304 zero
    // baseline. This does not claim original life-count semantics.
    response = GameRespawnNotification{request.parameters, 0, 0}.Encode();
  } else if (packet.message_id ==
             GameRespawnCompletionNotification::kMessageId) {
    const auto report = GameRespawnCompletionNotification::Decode(packet);
    actor = report.parameters.actor_object_id;
    if (report.parameters.value_at4 || report.parameters.value_at12 ||
        report.byte_at20 != 0 || report.value_at21 != 0) {
      throw std::runtime_error("Unsupported ordinary-room respawn completion.");
    }
    response = packet;
  } else {
    throw std::runtime_error("Unsupported room game message.");
  }
  CheckActor(slot, actor);
  // Keep parsing/ownership checks after returning, but never resurrect a
  // finished match or replay a notification into its waiting room.
  if (Ended()) {
    return {};
  }
  if (phase_ != RoomGamePhase::kPlaying) {
    throw std::runtime_error("Actor lifecycle before shared game start.");
  }
  auto& member = members_.at(slot);
  auto next = member.actor_phase;
  bool completed = member.respawn_completed;
  if (packet.message_id == GameActorDeathNotification::kMessageId) {
    if (next != RoomActorPhase::kAlive && next != RoomActorPhase::kBubble) {
      return {};
    }
    next = RoomActorPhase::kDown;
    completed = false;
  } else if (packet.message_id ==
             GameActorRespawnWaitNotification::kMessageId) {
    if (next == RoomActorPhase::kAlive || next == RoomActorPhase::kBubble) {
      throw std::runtime_error("Room respawn wait without death.");
    }
    if (next != RoomActorPhase::kDown) {
      return {};
    }
    next = RoomActorPhase::kWaiting;
  } else if (packet.message_id == GameRespawnRequest::kMessageId) {
    if (next == RoomActorPhase::kAwaitingCompletion) {
      return {};
    }
    if (next != RoomActorPhase::kWaiting) {
      throw std::runtime_error(
          "Room respawn request outside death/wait sequence.");
    }
    next = RoomActorPhase::kAwaitingCompletion;
  } else {
    if (next == RoomActorPhase::kAlive && completed) {
      return {};
    }
    if (next != RoomActorPhase::kAwaitingCompletion) {
      throw std::runtime_error("Unsolicited room respawn completion.");
    }
    next = RoomActorPhase::kAlive;
    completed = true;
  }
  // All clients apply death/wait/completion to the same scene actor.
  // S632 updates scene membership too; its native ownership predicate
  // restricts the follow-up completion sender. Never accept a peer's ID.
  std::vector<RoomGameDelivery> deliveries{{std::move(response), {}, {}}};
  if (packet.message_id == GameActorDeathNotification::kMessageId) {
    AppendScore(deliveries, ScoreEvent::kDeath, slot, slot);
  }
  member.actor_phase = next;
  if (next == RoomActorPhase::kDown || next == RoomActorPhase::kAlive) {
    ResetMovement(member);
  }
  ClearBubbleLinks(slot);
  member.respawn_completed = completed;
  member.damage_sources.clear();
  return deliveries;
}

void RoomGameSession::ClearBubbleLinks(std::uint32_t slot) noexcept {
  members_.at(slot).bubble_settled = false;
  members_.at(slot).action.reset();
  members_.at(slot).stomp.reset();
  for (auto& [other, member] : members_) {
    for (auto* pending : {&member.pop, &member.rescue, &member.rescue_effect}) {
      if (other == slot || (*pending && (*pending)->source_slot == slot)) {
        pending->reset();
      }
    }
  }
}

std::vector<RoomGameDelivery> RoomGameSession::ProcessActorAction(
    std::uint32_t slot, const protocol::TcpPacket& packet) {
  using namespace protocol;
  std::uint32_t actor = 0;
  bool valid = true;
  switch (packet.message_id) {
    case ClientToServerMessage0x800B::kMessageId: {
      const auto data = ClientToServerMessage0x800B::Decode(packet);
      actor = data.actor_object_id;
      valid = data.value_at4 <= 1;
      break;
    }
    case ClientToServerMessage0x800C::kMessageId: {
      const auto data = ClientToServerMessage0x800C::Decode(packet);
      actor = data.actor_object_id;
      // 8ED930 combines the two signed input axes with actor basis
      // vectors. Preserve the separate DWORD, whose complete role is
      // not established; it grants no position, damage or HP authority.
      const auto axis = [](std::uint8_t value) {
        return value <= 1 || value == 255;
      };
      valid = axis(data.axis_at8) && axis(data.axis_at9);
      break;
    }
    case ClientToServerMessage0x800D::kMessageId: {
      const auto data = ClientToServerMessage0x800D::Decode(packet);
      actor = data.actor_object_id;
      valid = data.value_at8 <= 1 &&
              (data.value_bits_at4 & 0x7F800000U) != 0x7F800000U;
      break;
    }
    case ClientToServerMessage0x800E::kMessageId:
      actor = ClientToServerMessage0x800E::Decode(packet).actor_object_id;
      break;
    case ClientToServerMessage0x802D::kMessageId:
      actor = ClientToServerMessage0x802D::Decode(packet).actor_object_id;
      break;
    default:
      throw std::invalid_argument("Unsupported actor action.");
  }
  CheckActor(slot, actor);
  if (!valid) {
    throw std::runtime_error("Invalid ordinary actor action fields.");
  }
  if (Ended() || phase_ != RoomGamePhase::kPlaying ||
      members_.at(slot).actor_phase != RoomActorPhase::kAlive) {
    return {};
  }
  if (packet.message_id == ClientToServerMessage0x802D::kMessageId) {
    members_.at(slot).stomp.reset();
  }
  // Native receivers also clear the owner's pending action flags. The
  // client remains responsible for animation and movement integration.
  return {{packet, {}, {}}};
}
}  // namespace bubble_fighter::server::features::rooms
