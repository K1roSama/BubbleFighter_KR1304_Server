#include "protocol/unresolved/server_to_client_message_0x025e.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ServerToClientMessage0x025E::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(value_at0);
  return {kMessageId, writer.TakeBytes()};
}

ServerToClientMessage0x025E ServerToClientMessage0x025E::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid S606 message.");
  }
  PacketReader reader(packet.payload);
  ServerToClientMessage0x025E result;
  result.value_at0 = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
