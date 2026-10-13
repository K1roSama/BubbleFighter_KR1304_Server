#include "protocol/characters/create_user_info_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket CreateUserInfoRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUtf16(character_name);
  writer.WriteUInt32(character_type);
  writer.WriteUInt32(color_index);
  writer.WriteUtf16(unknown_string);
  writer.WriteUInt32(unknown_value);
  return {kMessageId, writer.TakeBytes()};
}

CreateUserInfoRequest CreateUserInfoRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected create-user request message id.");
  }
  PacketReader reader(packet.payload);
  CreateUserInfoRequest message;
  message.character_name = reader.ReadUtf16();
  message.character_type = reader.ReadUInt32();
  message.color_index = reader.ReadUInt32();
  message.unknown_string = reader.ReadUtf16();
  message.unknown_value = reader.ReadUInt32();
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
