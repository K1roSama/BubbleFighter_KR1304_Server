#include "protocol/unresolved/client_to_server_message_0x0260.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x0260::Encode() const {
  PacketWriter writer;
  writer.WriteUInt8(event_code);
  writer.WriteUInt32(value_at1);
  writer.WriteUInt32(value_at5);
  writer.WriteUInt8(byte_at9);
  writer.WriteUInt8(byte_at10);
  writer.WriteUInt8(byte_at11);
  writer.WriteUInt16(short_at12);
  writer.WriteUInt16(short_at14);
  return {kMessageId, writer.TakeBytes()};
}

ClientToServerMessage0x0260 ClientToServerMessage0x0260::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid C608 message id.");
  }
  PacketReader reader(packet.payload);
  const ClientToServerMessage0x0260 result{
      reader.ReadUInt8(),  reader.ReadUInt32(), reader.ReadUInt32(),
      reader.ReadUInt8(),  reader.ReadUInt8(),  reader.ReadUInt8(),
      reader.ReadUInt16(), reader.ReadUInt16()};
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
