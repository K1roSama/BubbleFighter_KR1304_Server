#include "protocol/unresolved/client_to_server_message_0x0270.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x0270::Encode() const {
  if (value_at4 > 255) {
    throw std::invalid_argument(
        "C624 second field exceeds native byte domain.");
  }
  PacketWriter writer;
  writer.WriteUInt32(value_at0);
  writer.WriteUInt32(value_at4);
  return {kMessageId, writer.TakeBytes()};
}
ClientToServerMessage0x0270 ClientToServerMessage0x0270::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid C624 message.");
  }
  PacketReader reader(packet.payload);
  ClientToServerMessage0x0270 result{reader.ReadUInt32(), reader.ReadUInt32()};
  reader.RequireEnd();
  if (result.value_at4 > 255) {
    throw std::invalid_argument(
        "C624 second field exceeds native byte domain.");
  }
  return result;
}
}  // namespace bubble_fighter::server::protocol
