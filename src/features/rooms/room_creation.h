#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_CREATION_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_CREATION_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "features/characters/character_store.h"
#include "features/rooms/room_registry.h"
#include "features/rooms/room_roster.h"
#include "protocol/rooms/create_room_request.h"
#include "protocol/rooms/create_room_response.h"
#include "protocol/scenes/goto_room_notification.h"

namespace bubble_fighter::server::features::rooms {
// Local server policy. None of these values were recovered from the
// original server; an empty or zero policy rejects every request.
struct RoomCreationPolicy {
  // Configuration.ShortAt4 values (0x00856047: dialog +0x720) that this
  // server is prepared to host. The dialog offers the hard-coded table
  // built at 0x005C6580; large-room modes are always refused.
  std::vector<std::uint16_t> playable_modes;
  // Configuration.ValuesAt20[0] from 0x005C8AC0: chkOption[0] -> 1
  // (hammer battle), else [2] -> 3 (speed pop), else [5] -> 4 (map
  // view), else 0. Labels: Ui/Stages/Lobby/DlgCreateRoom_add_survival.ui.
  std::vector<std::uint32_t> room_kinds;
  std::size_t max_title_code_units = 0;
  std::size_t max_password_code_units = 0;
};

// Trusted state of an authenticated character already in the plaza; the
// same member/equipment snapshot the plaza S117 was built from.
struct RoomCreationInput {
  std::uint64_t authenticated_account_id = 0;
  characters::CharacterRecord character;
  protocol::SessionMember member;
  protocol::PartySupplementaryRecord equipment;
  // The creator's connection; required (RoomRegistry.h).
  RoomPacketSink sink;
};

struct RoomCreationOutcome {
  protocol::CreateRoomResult result = protocol::CreateRoomResult::kFailed;
  // Refusals: the S102 to return to the requester. On success this is
  // empty: S102 and the room S117 went to the sink when the room was
  // published, ahead of any member's broadcast.
  std::vector<protocol::TcpPacket> responses;
  // Present only on success; the session keeps it while inside the room.
  RoomLease room;
};

class RoomCreation {
 public:
  // 0x009D9250: a type-2 context bounds slot access with 16 entries
  // unless 0x01150EF0 selects the 36-slot variant.
  static constexpr std::uint32_t kOrdinaryRoomSlots = 16;
  // Lobby dialog 0x00855AE0 only ORs these bits into Configuration.ValueAt0:
  // 0x1 chkOption[1] (round battle) or mode 19, 0x20 chkOption[3] (random
  // teams), 0x80 chkOption[7] (champions cup rule), 0x800 channel type 80
  // (0x00896870). chkOption[6] (organize contest) is the contest byte.
  static constexpr std::uint32_t kOrdinaryOptionFlags =
      0x0001 | 0x0020 | 0x0080 | 0x0800;
  // wgtPvp (0x01134B50): the dialog's default entry (0x005C58F4) and
  // enabled in every channel type (0x010E57D0 / 0x010E5870).
  static constexpr std::uint16_t kStandardPvpMode = 1;
  // 0x005C8686 caps edtRoomTitle at 20 code units (UiEdit MaxLength,
  // +0x4F0 per 0x00FCA7EC), overriding the resource value 15.
  static constexpr std::size_t kDialogTitleCodeUnits = 20;
  // DlgCreateRoom_add_survival.ui: edtPassword MaxLength="6".
  static constexpr std::size_t kDialogPasswordCodeUnits = 6;
  // Local policy: the creator becomes the host in the first slot.
  static constexpr std::uint32_t kHostSlot = 0;

  // Malformed requests (values the client writer cannot produce) and
  // invalid trusted input throw without touching the registry. Policy
  // refusals return only S102. Success commits the room after both
  // packets were fully encoded; any exception rolls the claim back.
  // The room starts waiting (descriptor ValueAt4 = 2, RoomCreation R20).
  static RoomCreationOutcome Create(RoomRegistry& registry,
                                    const RoomCreationPolicy& policy,
                                    const protocol::CreateRoomRequest& request,
                                    RoomCreationInput input);

  // 0x01150EF0: modes that may switch the room context to 36 slots.
  static bool IsLargeRoomMode(std::uint16_t mode) noexcept;
};
}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_CREATION_H_
