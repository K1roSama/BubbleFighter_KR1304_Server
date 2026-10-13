#ifndef BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_INITIAL_HANDSHAKE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_INITIAL_HANDSHAKE_H_

#include <vector>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::features::authentication {
enum class InitialHandshakeState {
  kAwaitingInitialRecord,
  kInitialRecordAccepted,
  kRejected
};

class InitialHandshake {
 public:
  std::vector<protocol::TcpPacket> Process(const protocol::TcpPacket& packet);
  InitialHandshakeState State() const;

 private:
  InitialHandshakeState state_ = InitialHandshakeState::kAwaitingInitialRecord;
};
}  // namespace bubble_fighter::server::features::authentication

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_INITIAL_HANDSHAKE_H_
