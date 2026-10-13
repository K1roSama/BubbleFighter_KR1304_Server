#include "protocol/login/client_ping_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientPingResponse::Encode() const { return {kMessageId, {}}; }

ClientPingResponse ClientPingResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected client ping message ID.");
  }
  PacketReader(packet.payload).RequireEnd();
  return {};
}
}  // namespace bubble_fighter::server::protocol
