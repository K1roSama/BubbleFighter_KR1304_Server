#include "protocol/unresolved/client_to_server_message_0x800b.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x800B::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt32(value_at4);
  return {kMessageId, writer.TakeBytes()};
}
ClientToServerMessage0x800B ClientToServerMessage0x800B::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid actor action 0x800B id.");
  }
  PacketReader reader(packet.payload);
  ClientToServerMessage0x800B value{reader.ReadUInt32(), reader.ReadUInt32()};
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
