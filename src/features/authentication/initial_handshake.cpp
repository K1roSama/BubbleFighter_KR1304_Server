#include "features/authentication/initial_handshake.h"

#include <algorithm>
#include <stdexcept>

#include "protocol/login/center_list_notification.h"
#include "protocol/login/client_to_server_message_0x0001.h"
#include "protocol/login/game_patch_response.h"

namespace bubble_fighter::server::features::authentication {
std::vector<protocol::TcpPacket> InitialHandshake::Process(
    const protocol::TcpPacket& packet) {
  if (state_ == InitialHandshakeState::kRejected) {
    throw std::runtime_error("Handshake has already failed.");
  }
  if (packet.message_id != protocol::ClientToServerMessage0x0001::kMessageId) {
    // Other connection variants and login messages are not handled yet.
    // No authentication state is granted by this transport observation.
    return {};
  }
  if (state_ != InitialHandshakeState::kAwaitingInitialRecord) {
    state_ = InitialHandshakeState::kRejected;
    throw std::runtime_error("Duplicate initial record.");
  }
  try {
    const auto message = protocol::ClientToServerMessage0x0001::Decode(packet);
    // E008: exact initial profile emitted by the analyzed normal branch.
    // The first field's business meaning is still unknown.
    const bool supported =
        message.initial_value == 1256 && message.intermediate_value == 0 &&
        std::all_of(message.string_fields.begin(), message.string_fields.end(),
                    [](const auto& field) { return field.empty(); });
    if (!supported) {
      state_ = InitialHandshakeState::kRejected;
      return {protocol::GamePatchResponse{1, u"Unsupported initial record."}
                  .Encode()};
    }
    // E033: an empty gspCenterList explicitly selects the client's
    // single-center branch (0x881500) and releases its startup wait.
    std::vector<protocol::TcpPacket> responses{
        protocol::GamePatchResponse{0, {}}.Encode(),
        protocol::CenterListNotification{}.Encode()};
    state_ = InitialHandshakeState::kInitialRecordAccepted;
    return responses;
  } catch (...) {
    state_ = InitialHandshakeState::kRejected;
    throw;
  }
}

InitialHandshakeState InitialHandshake::State() const { return state_; }
}  // namespace bubble_fighter::server::features::authentication
