#include "features/rooms/room_roster.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "features/scenes/scene_roster.h"

namespace bubble_fighter::server::features::rooms {
protocol::SessionDescriptor RoomRoster::Describe(const RoomRecord& room) {
  if (room.occupants.empty() || room.slot_count == 0 ||
      room.slot_count > features::scenes::SceneRoster::kMaxSlots) {
    throw std::invalid_argument(
        "Room record has no members or an unsupported slot count.");
  }
  auto descriptor = room.settings;
  descriptor.short_at0 = room.room_id;
  descriptor.value_at4 = static_cast<std::uint32_t>(room.state);
  descriptor.session_type = 2;
  // Local choice: members over room capacity, nothing subtracted. The
  // creation request leaves all three at zero (RoomCreation R15).
  descriptor.shorts_at32 = {static_cast<std::uint16_t>(room.occupants.size()),
                            0, static_cast<std::uint16_t>(room.slot_count)};
  return descriptor;
}

protocol::GotoRoomNotification RoomRoster::Snapshot(const RoomRecord& room,
                                                    std::uint32_t local_slot) {
  const auto* local = room.FindSlot(local_slot);
  if (!local) {
    throw std::invalid_argument("Room snapshot for an empty slot.");
  }
  protocol::GotoRoomNotification envelope;
  envelope.descriptor = Describe(room);
  // 0x009EBF60 compares this host slot with the local slot (+728).
  envelope.value_at544 = room.host_slot;
  envelope.team = room.team;
  // Members are positional: the vector reaches the highest occupied
  // slot and empty entries stay default (0x009D9250 copies by index).
  std::vector<features::scenes::SceneSlot> slots(room.occupants.back().slot +
                                                 1);
  for (const auto& occupant : room.occupants) {
    auto& slot = slots.at(occupant.slot);
    slot.state = occupant.member;
    slot.character = occupant.character;
    slot.equipment = occupant.equipment;
  }
  return features::scenes::SceneRoster::ComposeRoom(
      std::move(envelope), slots, local_slot, local->account_id);
}

protocol::RoomSlotUpdateNotification RoomRoster::SlotUpdate(
    const RoomRecord& room, std::uint32_t slot) {
  auto snapshot = Snapshot(room, slot);
  protocol::RoomSlotUpdateNotification update;
  update.slot = slot;
  update.member = std::move(snapshot.members.at(slot));
  // SceneRoster keys +564 by slot and +588 by player identity.
  const auto by_slot =
      std::find_if(snapshot.records_at564.begin(), snapshot.records_at564.end(),
                   [&](const auto& record) { return record.key == slot; });
  const auto by_player =
      std::find_if(snapshot.records_at588.begin(), snapshot.records_at588.end(),
                   [&](const auto& record) {
                     return record.key == update.member.value_at8;
                   });
  if (by_slot == snapshot.records_at564.end() ||
      by_player == snapshot.records_at588.end()) {
    throw std::logic_error("Room snapshot lacks the slot's equipment records.");
  }
  update.records_at564.push_back(std::move(*by_slot));
  update.records_at588.push_back(std::move(*by_player));
  update.team = room.team;
  return update;
}

protocol::RoomMemberLeftNotification RoomRoster::MemberLeft(
    const RoomRecord& room, std::uint32_t slot) {
  if (slot >= room.slot_count || room.FindSlot(slot)) {
    throw std::invalid_argument(
        "Member-left notification for an occupied or invalid slot.");
  }
  protocol::RoomMemberLeftNotification message;
  message.slot = slot;
  // Default member: BooleansAt0[0] (occupied) is 0 and +204 is absent.
  message.team = room.team;
  return message;
}
}  // namespace bubble_fighter::server::features::rooms
