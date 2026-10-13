#include "features/lobby/lobby_session.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace bubble_fighter::server::features::lobby {
protocol::GotoLobbyNotification LobbySession::RequestEntry(
    const protocol::ClientToServerMessage0x006A&) {
  // Local single-party flow: a manual request does not need a party-move
  // notice. This is not proof of original response ordering or plaza data.
  state_ = LobbyEntryState::kNotificationSent;
  return {0};
}

protocol::TcpPacket LobbySession::RequestEntry(
    const protocol::ClientToServerMessage0x006A&,
    protocol::GotoRoomNotification scene, scenes::SceneSlot local_player,
    std::uint64_t authenticated_account_id) {
  // BFCLIENT02 0x009AC490 case 3 installs the shared model, copies the
  // scene title and requests LobbyStage. No second stage-only packet.
  if (!local_player.character || !local_player.equipment ||
      local_player.equipment->key != local_player.character->character_id) {
    throw std::invalid_argument(
        "Lobby equipment does not belong to the local character.");
  }
  scene.descriptor.session_type = 3;
  std::array<scenes::SceneSlot, 1> slots{std::move(local_player)};
  auto snapshot = scenes::SceneRoster::Compose(std::move(scene), slots, 0,
                                               authenticated_account_id);
  auto packet = snapshot.Encode();
  state_ = LobbyEntryState::kSnapshotSent;
  return packet;
}

LobbyEntryState LobbySession::State() const noexcept { return state_; }
}  // namespace bubble_fighter::server::features::lobby
