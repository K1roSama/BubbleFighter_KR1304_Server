#ifndef BUBBLEFIGHTER_SERVER_FEATURES_LOBBY_LOBBY_SESSION_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_LOBBY_LOBBY_SESSION_H_
#include "features/scenes/scene_roster.h"
#include "protocol/scenes/goto_lobby_notification.h"
#include "protocol/unresolved/client_to_server_message_0x006a.h"

namespace bubble_fighter::server::features::lobby {
enum class LobbyEntryState {
  kAwaitingRequest,
  kNotificationSent,
  kSnapshotSent
};

// Owned only after the authenticated character profile and solo party exist.
class LobbySession {
 public:
  protocol::GotoLobbyNotification RequestEntry(
      const protocol::ClientToServerMessage0x006A&);
  // Full local solo-plaza path: S117 itself selects stage 3. Encoding
  // completes before publishing the new server-side state.
  protocol::TcpPacket RequestEntry(const protocol::ClientToServerMessage0x006A&,
                                   protocol::GotoRoomNotification scene,
                                   scenes::SceneSlot local_player,
                                   std::uint64_t authenticated_account_id);
  void SnapshotPublished() noexcept { state_ = LobbyEntryState::kSnapshotSent; }
  LobbyEntryState State() const noexcept;

 private:
  LobbyEntryState state_ = LobbyEntryState::kAwaitingRequest;
};
}  // namespace bubble_fighter::server::features::lobby

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_LOBBY_LOBBY_SESSION_H_
