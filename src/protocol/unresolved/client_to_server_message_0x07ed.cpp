#include "protocol/unresolved/client_to_server_message_0x07ed.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x07ED::Encode() const {
  PacketWriter writer;
  writer.WriteUInt8(event_code);
  writer.WriteUInt32(value_at1);
  writer.WriteUInt8(byte_at5);
  return {kMessageId, writer.TakeBytes()};
}

ClientToServerMessage0x07ED ClientToServerMessage0x07ED::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid C2029 message id.");
  }
  PacketReader reader(packet.payload);
  const ClientToServerMessage0x07ED result{
      reader.ReadUInt8(), reader.ReadUInt32(), reader.ReadUInt8()};
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
