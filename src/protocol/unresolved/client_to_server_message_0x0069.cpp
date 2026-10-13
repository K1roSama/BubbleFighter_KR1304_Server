#include "protocol/unresolved/client_to_server_message_0x0069.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x0069::Encode() const {
  return {kMessageId, {}};
}

ClientToServerMessage0x0069 ClientToServerMessage0x0069::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId || !packet.payload.empty()) {
    throw std::invalid_argument("Invalid C105 empty message.");
  }
  return {};
}
}  // namespace bubble_fighter::server::protocol
