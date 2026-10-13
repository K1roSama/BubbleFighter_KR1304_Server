#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_ACTIONS_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_ACTIONS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "features/characters/character_store.h"
#include "features/rooms/room_registry.h"
#include "protocol/battle/game_load_notification.h"
#include "protocol/rooms/join_room_request.h"
#include "protocol/rooms/join_room_response.h"
#include "protocol/rooms/room_list_request.h"
#include "protocol/rooms/room_list_response.h"
#include "protocol/rooms/room_ready_request.h"
#include "protocol/rooms/start_game_request.h"

namespace bubble_fighter::server::features::rooms {
// Local server policy, not recovered from the original server.
struct RoomPlayPolicy {
  // Members (host included) required before a start is accepted.
  // User-authorized local solo play; other occupants must still be ready.
  std::size_t minimum_players_to_start = 1;
  // Bound on each forwarded C503 map text; the client loads MapResource.
  std::size_t max_map_text_code_units = 260;
};

// Same trusted plaza snapshot a room creator supplies (RoomCreationInput).
struct RoomJoinInput {
  std::uint64_t authenticated_account_id = 0;
  characters::CharacterRecord character;
  protocol::SessionMember member;
  protocol::PartySupplementaryRecord equipment;
  RoomPacketSink sink;
};

struct RoomJoinOutcome {
  protocol::JoinRoomResult result = protocol::JoinRoomResult::kNoMatchingRoom;
  // Refusals: the S103 to return to the requester. Empty when joined:
  // S103 and the room S117 went to the sink inside the registry.
  std::vector<protocol::TcpPacket> responses;
  RoomLease room;
};

class RoomActions {
 public:
  // C401 -> S101. Throws for page sizes the client writer cannot send.
  static protocol::RoomListResponse List(
      const RoomRegistry& registry, const protocol::RoomListRequest& request);
  // C403 -> S103 (+ S117 and the members' S503 when joined).
  static RoomJoinOutcome Join(RoomRegistry& registry,
                              const protocol::JoinRoomRequest& request,
                              RoomJoinInput input);
  // C505 -> S503 to every member. Returns false while the room plays.
  static bool SetReady(RoomLease& room,
                       const protocol::RoomReadyRequest& request);
  // C503 -> S510 to every member when accepted. Refusals send nothing:
  // no refusal message for this request is known, and the start button
  // retries after its 2000 ms cooldown (0x00970590).
  static RoomStartStatus Start(RoomLease& room, const RoomPlayPolicy& policy,
                               const protocol::StartGameRequest& request);
  // The S510 sent for an accepted C503 (RoomCreation R22).
  static protocol::GameLoadNotification CreateGameLoad(
      const RoomPlayPolicy& policy, const protocol::StartGameRequest& request);
};
}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_ACTIONS_H_
