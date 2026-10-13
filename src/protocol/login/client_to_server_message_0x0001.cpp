#include "protocol/login/client_to_server_message_0x0001.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x0001::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(initial_value);
  for (std::size_t index = 0; index < string_fields.size(); ++index) {
    if (index == 7) {
      writer.WriteUInt32(intermediate_value);
    }
    writer.WriteUtf16(string_fields[index]);
  }
  return {kMessageId, writer.TakeBytes()};
}

ClientToServerMessage0x0001 ClientToServerMessage0x0001::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected initial message id.");
  }
  PacketReader reader(packet.payload);
  ClientToServerMessage0x0001 message;
  message.initial_value = reader.ReadUInt32();
  for (std::size_t index = 0; index < message.string_fields.size(); ++index) {
    if (index == 7) {
      message.intermediate_value = reader.ReadUInt32();
    }
    message.string_fields[index] = reader.ReadUtf16();
  }
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
