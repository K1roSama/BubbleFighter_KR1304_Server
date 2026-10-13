#include "protocol/unresolved/client_to_server_message_0x800c.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x800C::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt32(value_at4);
  writer.WriteUInt8(axis_at8);
  writer.WriteUInt8(axis_at9);
  return {kMessageId, writer.TakeBytes()};
}
ClientToServerMessage0x800C ClientToServerMessage0x800C::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid actor action 0x800C id.");
  }
  PacketReader reader(packet.payload);
  ClientToServerMessage0x800C value{reader.ReadUInt32(), reader.ReadUInt32(),
                                    reader.ReadUInt8(), reader.ReadUInt8()};
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
