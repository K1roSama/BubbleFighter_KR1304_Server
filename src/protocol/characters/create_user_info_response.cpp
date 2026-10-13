#include "protocol/characters/create_user_info_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket CreateUserInfoResponse::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(result);
  return {kMessageId, writer.TakeBytes()};
}

CreateUserInfoResponse CreateUserInfoResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected create-user response message id.");
  }
  PacketReader reader(packet.payload);
  CreateUserInfoResponse message;
  message.result = reader.ReadUInt32();
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
