#include "features/rooms/room_creation.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

#include "features/rooms/room_roster.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::features::rooms {
namespace {
template <typename Value>
bool Contains(const std::vector<Value>& values, Value value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

void ValidatePolicy(const RoomCreationPolicy& policy) {
  if (std::any_of(policy.playable_modes.begin(), policy.playable_modes.end(),
                  RoomCreation::IsLargeRoomMode) ||
      policy.max_title_code_units >
          protocol::PacketWriter::kMaxStringCodeUnits ||
      policy.max_password_code_units >
          protocol::PacketWriter::kMaxStringCodeUnits) {
    throw std::invalid_argument(
        "Room creation policy exceeds the supported room shapes.");
  }
}

void ValidateRequestShape(const protocol::CreateRoomRequest& request) {
  // 0x009622A3 clears ShortAt0 and no creation path assigns
  // SessionType after 0x0113B7B0 sets it to 0. The server owns both.
  if (request.settings.short_at0 != 0 || request.settings.session_type != 0) {
    throw std::invalid_argument(
        "Create-room request carries a server-owned room identity.");
  }
  // 0x00962272..0x00962284: the flag is exactly "password not empty".
  if (request.settings.boolean_at38 != (request.password.empty() ? 0U : 1U)) {
    throw std::invalid_argument(
        "Create-room password flag does not match the password.");
  }
}

RoomCreationOutcome Refuse(protocol::CreateRoomResult result) {
  RoomCreationOutcome outcome;
  outcome.result = result;
  outcome.responses.push_back(
      protocol::CreateRoomResponse{0, static_cast<std::uint32_t>(result), 0}
          .Encode());
  return outcome;
}
}  // namespace

bool RoomCreation::IsLargeRoomMode(std::uint16_t mode) noexcept {
  // 0x01150EF0 returns true for 32, and for these when 0x01150F80 is
  // false. That runtime condition is unknown, so all are treated as large.
  switch (mode) {
    case 0x17:
    case 0x19:
    case 0x1E:
    case 0x20:
    case 0x2A:
    case 0x2C:
    case 0x2D:
      return true;
    default:
      return false;
  }
}

RoomCreationOutcome RoomCreation::Create(
    RoomRegistry& registry, const RoomCreationPolicy& policy,
    const protocol::CreateRoomRequest& request, RoomCreationInput input) {
  using protocol::CreateRoomResult;
  ValidatePolicy(policy);
  ValidateRequestShape(request);
  const auto& character = input.character;
  if (input.authenticated_account_id == 0 ||
      character.account_id != input.authenticated_account_id ||
      character.character_id == 0 ||
      character.character_id > std::numeric_limits<std::uint32_t>::max() ||
      input.equipment.key != character.character_id || !input.sink) {
    throw std::invalid_argument(
        "Room host does not belong to the authenticated account.");
  }

  const auto& settings = request.settings;
  const auto& configuration = settings.configuration;
  if (IsLargeRoomMode(configuration.short_at4) ||
      !Contains(policy.playable_modes, configuration.short_at4)) {
    return Refuse(CreateRoomResult::kModeUnavailable);
  }
  // Unsupported creation variants: contest rooms (0x00856146 branch on
  // BooleanAt8), other dialogs' flag bits or trailing value, and local
  // title/password limits. They are refused rather than reinterpreted.
  if (request.trailing_value || configuration.boolean_at8 != 0 ||
      configuration.value_at0 != 0 ||
      !Contains(policy.room_kinds, configuration.values_at20[0]) ||
      settings.text_at8.empty() ||
      settings.text_at8.size() > policy.max_title_code_units ||
      request.password.size() > policy.max_password_code_units) {
    return Refuse(CreateRoomResult::kFailed);
  }

  // Local policy: one room per account/character; a repeated request
  // while a room or claim exists is refused without changing it.
  auto reserved =
      registry.Reserve(input.authenticated_account_id, character.character_id);
  if (reserved.status != RoomReserveStatus::kReserved) {
    return Refuse(CreateRoomResult::kFailed);
  }
  auto& reservation = *reserved.reservation;

  // The host's chosen settings are kept; RoomRoster::Describe assigns
  // the server-owned room number (C403 identity, 0x00855310), state,
  // counts and S117 type whenever a descriptor is sent.
  RoomRecord record;
  record.room_id = reservation.RoomId();
  record.slot_count = kOrdinaryRoomSlots;
  record.host_slot = kHostSlot;
  record.state = RoomState::kWaiting;
  record.settings = settings;
  record.password = request.password;
  RoomOccupant host;
  host.slot = kHostSlot;
  host.account_id = input.authenticated_account_id;
  host.character = std::move(input.character);
  host.member = std::move(input.member);
  // 96DD20 accepts active teams 1/2; 36 is reserved for spectators.
  host.member.signed_byte_at4 = 1;
  host.member.booleans_at96 = {};
  host.equipment = std::move(input.equipment);
  record.occupants.push_back(std::move(host));

  // 0x00856F14 marks this client as creator before S117 builds stage 4.
  // Only the host entry is sent; 0x009D9250 default-constructs the other
  // slots and copies members by index below the 16-slot bound.
  std::vector<protocol::TcpPacket> packets;
  packets.push_back(protocol::CreateRoomResponse{}.Encode());
  packets.push_back(RoomRoster::Snapshot(record, kHostSlot).Encode());

  RoomCreationOutcome outcome;
  outcome.result = CreateRoomResult::kCreated;
  outcome.room =
      std::move(reservation)
          .Commit(std::move(record), std::move(input.sink), std::move(packets));
  return outcome;
}
}  // namespace bubble_fighter::server::features::rooms
