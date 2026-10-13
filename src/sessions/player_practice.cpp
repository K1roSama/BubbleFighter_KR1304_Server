#include <optional>
#include <stdexcept>
#include <vector>

#include "features/lobby/lobby_session.h"
#include "features/practice/practice_entry.h"
#include "protocol/battle/game_actor_death_notification.h"
#include "protocol/battle/game_actor_respawn_wait_notification.h"
#include "protocol/battle/game_extended_shot_notification.h"
#include "protocol/battle/game_loading_complete_request.h"
#include "protocol/battle/game_movement_notification.h"
#include "protocol/battle/game_report_request.h"
#include "protocol/battle/game_respawn_completion_notification.h"
#include "protocol/battle/game_respawn_request.h"
#include "protocol/battle/game_shot_notification.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/unresolved/client_to_server_message_0x006b.h"
#include "protocol/unresolved/client_to_server_message_0x0243.h"
#include "protocol/unresolved/client_to_server_message_0x025f.h"
#include "protocol/unresolved/client_to_server_message_0x0260.h"
#include "protocol/unresolved/client_to_server_message_0x0270.h"
#include "protocol/unresolved/client_to_server_message_0x07ed.h"
#include "sessions/player_session.h"

namespace bubble_fighter::server::sessions {
std::optional<std::vector<protocol::TcpPacket>> PlayerSession::ProcessPractice(
    const protocol::TcpPacket& packet) {
  using namespace protocol;
  if (packet.message_id == ClientToServerMessage0x006B::kMessageId) {
    const auto request = ClientToServerMessage0x006B::Decode(packet);
    if (state_ != PlayerSessionState::kMenuInitializationSent || !account_id_ ||
        !character_ || !menu_data_ || !lobby_ ||
        lobby_->State() != features::lobby::LobbyEntryState::kAwaitingRequest) {
      throw std::runtime_error(
          "Practice entry outside fully initialized menu.");
    }
    features::practice::PracticeEntry::Validate(
        request);  // Reject before loading persistence.
    const auto observed_at = InventoryObservationTime();
    auto responses = features::practice::PracticeEntry::Prepare(
        request, *character_, *account_id_,
        menu_data_(*character_, observed_at), observed_at);
    practice_.emplace(practice_clock_(), practice_seed_());
    state_ = PlayerSessionState::kPracticeLoading;
    return responses;
  }
  if (packet.message_id == ClientToServerMessage0x0243::kMessageId ||
      packet.message_id == ClientToServerMessage0x025F::kMessageId) {
    if (!account_id_ || !character_ || !practice_) {
      throw std::runtime_error("Loading data without an owned practice scene.");
    }
    const auto now = practice_clock_();
    if (packet.message_id == ClientToServerMessage0x0243::kMessageId) {
      practice_->SubmitLoadingData(ClientToServerMessage0x0243::Decode(packet),
                                   now);
    } else {
      practice_->SubmitLoadingActor(ClientToServerMessage0x025F::Decode(packet),
                                    now);
    }
    return std::vector<protocol::TcpPacket>{};  // No inferred success reply or
                                                // persistence side effect.
  }
  if (packet.message_id == GameLoadingCompleteRequest::kMessageId) {
    const auto report = GameLoadingCompleteRequest::Decode(packet);
    if (!account_id_ || !character_ || !practice_) {
      throw std::runtime_error(
          "Unsupported or out-of-order practice loading report.");
    }
    auto responses = practice_->LoadingComplete(report, practice_clock_());
    UpdatePracticeState();
    return responses;
  }
  if (packet.message_id == ClientToServerMessage0x0270::kMessageId) {
    const auto request = ClientToServerMessage0x0270::Decode(packet);
    if (state_ != PlayerSessionState::kPracticePlaying || !practice_ ||
        request.value_at0 != 1 || request.value_at4 != 0) {
      throw std::runtime_error(
          "Unsupported training exit or invalid practice state.");
    }
    return ReturnFromPractice();
  }
  if (packet.message_id == ClientToServerMessage0x0260::kMessageId ||
      packet.message_id == ClientToServerMessage0x07ED::kMessageId) {
    if (!account_id_ || !character_ || !practice_) {
      throw std::runtime_error(
          "Action report without an owned practice scene.");
    }
    if (packet.message_id == ClientToServerMessage0x0260::kMessageId) {
      practice_->SubmitActionReport(ClientToServerMessage0x0260::Decode(packet),
                                    practice_clock_());
    } else {
      practice_->SubmitActionReport(ClientToServerMessage0x07ED::Decode(packet),
                                    practice_clock_());
    }
    return std::vector<protocol::TcpPacket>{};  // No guessed acknowledgement,
                                                // score, task reset or reward.
  }
  if (packet.message_id == GameMovementNotification::kMessageId) {
    if (!account_id_ || !character_ || !practice_) {
      throw std::runtime_error("Movement without an owned practice scene.");
    }
    practice_->SubmitMovement(GameMovementNotification::Decode(packet),
                              practice_clock_());
    return std::vector<protocol::TcpPacket>{};
  }
  if (packet.message_id == GameShotNotification::kMessageId ||
      packet.message_id == GameExtendedShotNotification::kMessageId) {
    if (!account_id_ || !character_ || !practice_) {
      throw std::runtime_error("Shot without an owned practice scene.");
    }
    if (packet.message_id == GameShotNotification::kMessageId) {
      return practice_->SubmitShot(GameShotNotification::Decode(packet),
                                   practice_clock_());
    }
    return practice_->SubmitShot(GameExtendedShotNotification::Decode(packet),
                                 practice_clock_());
  }
  if (packet.message_id == GameActorDeathNotification::kMessageId) {
    if (!account_id_ || !character_ || !practice_) {
      throw std::runtime_error("Actor death without an owned practice scene.");
    }
    return practice_->ReportActorDeath(
        GameActorDeathNotification::Decode(packet), practice_clock_());
  }
  if (packet.message_id == GameActorRespawnWaitNotification::kMessageId ||
      packet.message_id == GameRespawnRequest::kMessageId ||
      packet.message_id == GameRespawnCompletionNotification::kMessageId) {
    if (!account_id_ || !character_ || !practice_) {
      throw std::runtime_error(
          "Respawn message without an owned practice scene.");
    }
    if (packet.message_id == GameActorRespawnWaitNotification::kMessageId) {
      return practice_->ReportRespawnWait(
          GameActorRespawnWaitNotification::Decode(packet), practice_clock_());
    }
    if (packet.message_id == GameRespawnRequest::kMessageId) {
      return practice_->RequestRespawn(GameRespawnRequest::Decode(packet),
                                       practice_clock_());
    }
    return practice_->CompleteRespawn(
        GameRespawnCompletionNotification::Decode(packet), practice_clock_());
  }
  if (packet.message_id == GameReportRequest::kMessageId) {
    const auto report = GameReportRequest::Decode(packet);
    if (!practice_ || !account_id_ || !character_) {
      throw std::runtime_error("Game report without an owned practice scene.");
    }
    practice_->SubmitReport(report, practice_clock_());
    UpdatePracticeState();
    return std::vector<protocol::TcpPacket>{};  // Poll prepares the local
                                                // practice result; no database
                                                // writes.
  }
  return std::nullopt;
}
}  // namespace bubble_fighter::server::sessions
