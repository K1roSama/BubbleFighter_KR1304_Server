#include "protocol/unresolved/client_to_server_message_0x800d.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x800D::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt32(value_bits_at4);
  writer.WriteUInt32(value_at8);
  return {kMessageId, writer.TakeBytes()};
}
ClientToServerMessage0x800D ClientToServerMessage0x800D::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid actor action 0x800D id.");
  }
  PacketReader reader(packet.payload);
  ClientToServerMessage0x800D value{reader.ReadUInt32(), reader.ReadUInt32(),
                                    reader.ReadUInt32()};
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
