#include <chrono>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "features/inventory/inventory_refresh.h"
#include "features/lobby/lobby_session.h"
#include "features/rooms/room_actions.h"
#include "features/rooms/room_creation.h"
#include "features/rooms/room_game_session.h"
#include "features/scenes/scene_roster.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/inventory/inventory_refresh_request.h"
#include "protocol/rooms/create_room_request.h"
#include "protocol/rooms/join_room_request.h"
#include "protocol/rooms/leave_room_request.h"
#include "protocol/rooms/room_list_request.h"
#include "protocol/rooms/room_ready_request.h"
#include "protocol/rooms/start_game_request.h"
#include "protocol/unresolved/client_to_server_message_0x0270.h"
#include "sessions/menu_initialization.h"
#include "sessions/player_session.h"

namespace bubble_fighter::server::sessions {
std::vector<protocol::TcpPacket> PlayerSession::EnterPlaza(
    MenuInitializationData data) {
  features::scenes::SceneSlot local{std::move(data.member), *character_,
                                    std::move(data.equipment)};
  if (!plazas_) {
    return {lobby_->RequestEntry({}, std::move(data.scene), std::move(local),
                                 *account_id_)};
  }
  if (plaza_) {
    plaza_.Refresh(std::move(local), true);
  } else {
    plaza_ = plazas_->Enter(std::move(data.scene), std::move(local),
                            *account_id_, plaza_packets_->Sink());
  }
  lobby_->SnapshotPublished();
  return {};  // Ordered queue holds S117 before any subsequent plaza event.
}

std::optional<std::vector<protocol::TcpPacket>> PlayerSession::ProcessRoom(
    const protocol::TcpPacket& packet) {
  using namespace protocol;
  using namespace features::rooms;
  const bool entry = packet.message_id == RoomListRequest::kMessageId ||
                     packet.message_id == CreateRoomRequest::kMessageId ||
                     packet.message_id == JoinRoomRequest::kMessageId;
  const bool room_action = packet.message_id == RoomReadyRequest::kMessageId ||
                           packet.message_id == StartGameRequest::kMessageId ||
                           packet.message_id == LeaveRoomRequest::kMessageId;
  if (!entry && !room_action && !room_) {
    return std::nullopt;
  }
  if (!entry && !room_action && room_ &&
      !RoomGameSession::Handles(packet.message_id) &&
      packet.message_id != ClientToServerMessage0x0270::kMessageId &&
      packet.message_id != InventoryRefreshRequest::kMessageId) {
    return std::nullopt;
  }
  if (!rooms_ || !account_id_ || !character_ || !menu_data_ || !lobby_) {
    throw std::runtime_error(
        "Room operation before authenticated initialization.");
  }
  const auto load = [&] {
    const auto seconds =
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    if (seconds < 0) {
      throw std::runtime_error("Invalid room observation time.");
    }
    return std::pair{
        static_cast<std::uint64_t>(seconds),
        menu_data_(*character_, static_cast<std::uint64_t>(seconds))};
  };
  if (entry) {
    if (room_ || state_ != PlayerSessionState::kMenuInitializationSent ||
        lobby_->State() != features::lobby::LobbyEntryState::kSnapshotSent) {
      throw std::runtime_error("Room entry outside the initialized plaza.");
    }
    if (packet.message_id == RoomListRequest::kMessageId) {
      return std::vector<TcpPacket>{
          RoomActions::List(*rooms_, RoomListRequest::Decode(packet)).Encode()};
    }
    if (packet.message_id == CreateRoomRequest::kMessageId) {
      const auto request = CreateRoomRequest::Decode(packet);
      auto [observedAt, data] = load();
      (void)observedAt;
      RoomCreationPolicy policy{{RoomCreation::kStandardPvpMode},
                                {0},
                                RoomCreation::kDialogTitleCodeUnits,
                                RoomCreation::kDialogPasswordCodeUnits};
      auto outcome = RoomCreation::Create(
          *rooms_, policy, request,
          {*account_id_, *character_, std::move(data.member),
           std::move(data.equipment), room_packets_->Sink()});
      if (outcome.room) {
        plaza_.Release();
        plaza_packets_->Clear();
        room_ = std::move(outcome.room);
        state_ = PlayerSessionState::kInRoom;
      }
      return outcome.responses;
    }
    const auto request = JoinRoomRequest::Decode(packet);
    auto [observedAt, data] = load();
    (void)observedAt;
    auto outcome =
        RoomActions::Join(*rooms_, request,
                          {*account_id_, *character_, std::move(data.member),
                           std::move(data.equipment), room_packets_->Sink()});
    if (outcome.room) {
      plaza_.Release();
      plaza_packets_->Clear();
      room_ = std::move(outcome.room);
      state_ = PlayerSessionState::kInRoom;
    }
    return outcome.responses;
  }
  if (!room_ || state_ != PlayerSessionState::kInRoom) {
    throw std::runtime_error("Room action outside an owned room.");
  }
  if (packet.message_id == RoomReadyRequest::kMessageId) {
    RoomActions::SetReady(room_, RoomReadyRequest::Decode(packet));
  } else if (packet.message_id == StartGameRequest::kMessageId) {
    RoomActions::Start(room_, RoomPlayPolicy{},
                       StartGameRequest::Decode(packet));
  } else if (packet.message_id == LeaveRoomRequest::kMessageId) {
    LeaveRoomRequest::Decode(packet);
    auto [observedAt, data] = load();
    (void)observedAt;
    room_.Release();
    room_packets_->Clear();
    auto responses = EnterPlaza(std::move(data));
    state_ = PlayerSessionState::kMenuInitializationSent;
    return responses;
  } else if (packet.message_id == InventoryRefreshRequest::kMessageId) {
    InventoryRefreshRequest::Decode(packet);
    auto [observedAt, data] = load();
    auto packets = features::inventory::InventoryRefresh::Prepare(
        *character_, *account_id_, data.inventory, data.equipment, data.presets,
        observedAt);
    if (data.cash) {
      if (data.cash->character_id != character_->character_id) {
        throw std::runtime_error("Room cash owner mismatch.");
      }
      packets.push_back(data.cash->CreateNotification(*account_id_).Encode());
    }
    return packets;
  } else if (packet.message_id == ClientToServerMessage0x0270::kMessageId) {
    const auto request = ClientToServerMessage0x0270::Decode(packet);
    if (request.value_at0 != 1 || request.value_at4 != 0) {
      throw std::runtime_error("Unsupported room game exit.");
    }
    room_.ReturnToWaiting();
  } else {
    room_.ProcessGame(packet);
  }
  return std::vector<TcpPacket>{};
}
}  // namespace bubble_fighter::server::sessions
