#include "protocol/unresolved/client_to_server_message_0x800e.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x800E::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  return {kMessageId, writer.TakeBytes()};
}
ClientToServerMessage0x800E ClientToServerMessage0x800E::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid actor action 0x800E id.");
  }
  PacketReader reader(packet.payload);
  ClientToServerMessage0x800E value{reader.ReadUInt32()};
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
