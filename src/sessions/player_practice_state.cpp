#include <stdexcept>
#include <vector>

#include "features/practice/practice_session.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "sessions/menu_initialization.h"
#include "sessions/player_session.h"

namespace bubble_fighter::server::sessions {
std::vector<protocol::TcpPacket> PlayerSession::ReturnFromPractice() {
  if (!practice_ || !account_id_ || !character_ || !menu_data_ || !lobby_) {
    throw std::runtime_error("Missing owned practice context.");
  }
  const auto observed_at = InventoryObservationTime();
  auto packets = MenuInitialization::Prepare(
      *character_, *account_id_, menu_data_(*character_, observed_at),
      observed_at);
  practice_.reset();
  lobby_.emplace();
  state_ = PlayerSessionState::kMenuInitializationSent;
  return packets;
}

void PlayerSession::UpdatePracticeState() noexcept {
  if (!practice_) {
    return;
  }
  switch (practice_->Phase()) {
    case features::practice::PracticePhase::kLoading:
      state_ = PlayerSessionState::kPracticeLoading;
      break;
    case features::practice::PracticePhase::kCountdown:
      state_ = PlayerSessionState::kPracticeCountdown;
      break;
    case features::practice::PracticePhase::kPlaying:
      state_ = PlayerSessionState::kPracticePlaying;
      break;
    case features::practice::PracticePhase::kAwaitingReport:
      state_ = PlayerSessionState::kPracticeAwaitingReport;
      break;
    case features::practice::PracticePhase::kFinishing:
      state_ = PlayerSessionState::kPracticeFinishing;
      break;
    case features::practice::PracticePhase::kResult:
      state_ = PlayerSessionState::kPracticeResult;
      break;
  }
}
}  // namespace bubble_fighter::server::sessions
