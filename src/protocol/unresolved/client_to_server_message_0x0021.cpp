#include "protocol/unresolved/client_to_server_message_0x0021.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x0021::Encode() const {
  return {kMessageId, {}};
}

ClientToServerMessage0x0021 ClientToServerMessage0x0021::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected creation follow-up message id.");
  }
  PacketReader(packet.payload).RequireEnd();
  return {};
}
}  // namespace bubble_fighter::server::protocol
