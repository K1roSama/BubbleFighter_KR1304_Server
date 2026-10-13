#include "protocol/scenes/move_into_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
bool MoveIntoResponse::IsAccepted() const noexcept {
  return accepted_value != 0;
}

TcpPacket MoveIntoResponse::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(accepted_value);
  writer.WriteUInt32(channel_category);
  writer.WriteUInt32(unknown_connection_value);
  return {kMessageId, writer.TakeBytes()};
}

MoveIntoResponse MoveIntoResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected move-into response message id.");
  }
  PacketReader reader(packet.payload);
  MoveIntoResponse message;
  message.accepted_value = reader.ReadUInt32();
  message.channel_category = reader.ReadUInt32();
  message.unknown_connection_value = reader.ReadUInt32();
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
