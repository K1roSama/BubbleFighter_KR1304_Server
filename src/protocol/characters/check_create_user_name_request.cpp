#include "protocol/characters/check_create_user_name_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket CheckCreateUserNameRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUtf16(character_name);
  return {kMessageId, writer.TakeBytes()};
}

CheckCreateUserNameRequest CheckCreateUserNameRequest::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected name-check request message id.");
  }
  PacketReader reader(packet.payload);
  CheckCreateUserNameRequest message;
  message.character_name = reader.ReadUtf16();
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
