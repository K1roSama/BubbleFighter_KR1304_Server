#include "protocol/unresolved/client_to_server_message_0x0209.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x0209::Encode() const {
  return {kMessageId, {}};
}

ClientToServerMessage0x0209 ClientToServerMessage0x0209::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected C521 message ID.");
  }
  PacketReader(packet.payload).RequireEnd();
  return {};
}
}  // namespace bubble_fighter::server::protocol
