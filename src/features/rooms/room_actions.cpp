#include "features/rooms/room_actions.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "features/rooms/room_map_catalog.h"
#include "features/rooms/room_registry.h"
#include "protocol/battle/game_load_notification.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/rooms/join_room_request.h"
#include "protocol/rooms/join_room_response.h"
#include "protocol/rooms/room_list_request.h"
#include "protocol/rooms/room_list_response.h"
#include "protocol/rooms/room_ready_request.h"
#include "protocol/rooms/start_game_request.h"

namespace bubble_fighter::server::features::rooms {
namespace {
bool IsForwardableMapText(const std::u16string& text, std::size_t limit) {
  return !text.empty() && text.size() <= limit &&
         std::none_of(text.begin(), text.end(),
                      [](char16_t unit) { return unit < 0x20; });
}

std::vector<protocol::TcpPacket> Refuse(protocol::JoinRoomResult result) {
  return {protocol::JoinRoomResponse{static_cast<std::uint32_t>(result), 0}
              .Encode()};
}
}  // namespace

protocol::RoomListResponse RoomActions::List(
    const RoomRegistry& registry, const protocol::RoomListRequest& request) {
  // 0x00858E70 is the only writer and always sends 10.
  if (request.page_size != protocol::RoomListRequest::kClientPageSize) {
    throw std::invalid_argument(
        "Room-list page size cannot come from the client writer.");
  }
  auto page = registry.List(request.page, request.page_size);
  protocol::RoomListResponse response;
  response.page = request.page;
  response.has_next_page = page.has_next_page ? 1U : 0U;
  response.rooms = std::move(page.rooms);
  return response;
}

RoomJoinOutcome RoomActions::Join(RoomRegistry& registry,
                                  const protocol::JoinRoomRequest& request,
                                  RoomJoinInput input) {
  if (input.authenticated_account_id == 0 ||
      input.character.account_id != input.authenticated_account_id ||
      input.character.character_id == 0 ||
      input.character.character_id >
          std::numeric_limits<std::uint32_t>::max() ||
      input.equipment.key != input.character.character_id || !input.sink) {
    throw std::invalid_argument(
        "Room joiner does not belong to the authenticated account.");
  }
  RoomJoinOutcome outcome;
  // The client sign-extends a positive ShortAt0; anything else names no
  // room this server published.
  if (request.room_id == 0 || request.room_id > RoomRegistry::kMaxRoomId) {
    outcome.responses = Refuse(outcome.result);
    return outcome;
  }
  RoomOccupant joiner;
  joiner.account_id = input.authenticated_account_id;
  joiner.character = std::move(input.character);
  joiner.member = std::move(input.member);
  joiner.equipment = std::move(input.equipment);
  // A fresh member is not ready (RoomCreation R19 bytes +96..+98).
  joiner.member.booleans_at96 = {};
  // Code 0 shows nothing (0x00853E90); the room S117 then selects stage 4.
  std::vector<protocol::TcpPacket> prefix{
      protocol::JoinRoomResponse{}.Encode()};
  auto joined = registry.Join(static_cast<std::uint16_t>(request.room_id),
                              request.password, std::move(joiner),
                              std::move(input.sink), std::move(prefix));
  switch (joined.status) {
    case RoomJoinStatus::kJoined:
      outcome.result = protocol::JoinRoomResult::kJoined;
      outcome.room = std::move(joined.room);
      return outcome;
    case RoomJoinStatus::kPlaying:
      outcome.result = protocol::JoinRoomResult::kGameStarted;
      break;
    case RoomJoinStatus::kWrongPassword:
      outcome.result = protocol::JoinRoomResult::kWrongPassword;
      break;
    case RoomJoinStatus::kFull:
      outcome.result = protocol::JoinRoomResult::kRoomFull;
      break;
    case RoomJoinStatus::kNoSuchRoom:
      outcome.result = protocol::JoinRoomResult::kNoMatchingRoom;
      break;
  }
  outcome.responses = Refuse(outcome.result);
  return outcome;
}

bool RoomActions::SetReady(RoomLease& room,
                           const protocol::RoomReadyRequest& request) {
  // C505 order is {state 1, state 3, state 2}; the member bytes are
  // +96 state 1, +97 state 2, +98 state 3 (0x009DA951). Normalized to 1.
  const auto flag = [](std::uint32_t value) { return value != 0 ? 1U : 0U; };
  return room.SetMemberState(
      {flag(request.ready), flag(request.value_at8), flag(request.value_at4)});
}

protocol::GameLoadNotification RoomActions::CreateGameLoad(
    const RoomPlayPolicy& policy, const protocol::StartGameRequest& request) {
  // 0x009E5E70 starts nothing unless the key and the third text are
  // non-empty, and loads MapResource. Forwarded texts are bounded.
  if (!IsForwardableMapText(request.map_key, policy.max_map_text_code_units) ||
      !IsForwardableMapText(request.map_resource,
                            policy.max_map_text_code_units) ||
      !IsForwardableMapText(request.map_attribute,
                            policy.max_map_text_code_units)) {
    throw std::invalid_argument(
        "Start-game map selection cannot be forwarded.");
  }
  protocol::GameLoadNotification start;
  // E089's native binding: wire texts 0/24/48 -> context 208/232/184.
  // Start resolves the accepted resource to a concrete catalog key first.
  // 9E4D10/7603B0 use +184 for MapMeta lookup; forwarding Random here
  // would use +232 (the display name), yielding a null resource.
  start.texts_at0 = {request.map_resource, request.map_attribute,
                     request.map_key};
  start.value_at72 = static_cast<std::uint32_t>(RoomState::kLoading);
  // Remaining fields and maps: local zero/empty defaults (R22).
  return start;
}

RoomStartStatus RoomActions::Start(RoomLease& room,
                                   const RoomPlayPolicy& policy,
                                   const protocol::StartGameRequest& request) {
  if (policy.minimum_players_to_start == 0) {
    throw std::invalid_argument("Room start policy allows an empty room.");
  }
  // 0x010E49C0 yields an empty attribute when the resource lacks it; the
  // client would then ignore S510, so such a start is refused instead.
  if (!IsForwardableMapText(request.map_key, policy.max_map_text_code_units) ||
      !IsForwardableMapText(request.map_resource,
                            policy.max_map_text_code_units) ||
      !IsForwardableMapText(request.map_attribute,
                            policy.max_map_text_code_units)) {
    return RoomStartStatus::kUnsupportedMap;
  }
  const auto* map = RoomMapCatalog::Match(request);
  if (!map) {
    return RoomStartStatus::kUnsupportedMap;
  }
  auto resolved = request;
  resolved.map_key = map->key;
  const auto packet = CreateGameLoad(policy, resolved).Encode();
  return room.Start(packet, policy.minimum_players_to_start,
                    map->rules.values[7], map->key);
}
}  // namespace bubble_fighter::server::features::rooms
