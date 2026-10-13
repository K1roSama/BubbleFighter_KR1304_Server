#include "protocol/unresolved/client_to_server_message_0x006a.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x006A::Encode() const {
  return {kMessageId, {}};
}

ClientToServerMessage0x006A ClientToServerMessage0x006A::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected plaza request id.");
  }
  PacketReader(packet.payload).RequireEnd();
  return {};
}
}  // namespace bubble_fighter::server::protocol
