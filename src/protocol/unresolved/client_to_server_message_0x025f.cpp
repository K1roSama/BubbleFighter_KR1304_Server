#include "protocol/unresolved/client_to_server_message_0x025f.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x025F::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(value_at0);
  return {kMessageId, writer.TakeBytes()};
}
ClientToServerMessage0x025F ClientToServerMessage0x025F::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid C607 message id.");
  }
  PacketReader reader(packet.payload);
  const ClientToServerMessage0x025F result{reader.ReadUInt32()};
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
