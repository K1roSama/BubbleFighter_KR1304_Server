#include "protocol/login/server_ping_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"

namespace bubble_fighter::server::protocol {
TcpPacket ServerPingRequest::Encode() const { return {kMessageId, {}}; }

ServerPingRequest ServerPingRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected server ping message ID.");
  }
  PacketReader(packet.payload).RequireEnd();
  return {};
}
}  // namespace bubble_fighter::server::protocol
