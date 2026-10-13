#include "protocol/rooms/create_room_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket CreateRoomResponse::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(value_at0);
  writer.WriteUInt32(result);
  writer.WriteUInt32(detail_value);
  return {kMessageId, writer.TakeBytes()};
}

CreateRoomResponse CreateRoomResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected create-room response id.");
  }
  PacketReader reader(packet.payload);
  CreateRoomResponse response;
  response.value_at0 = reader.ReadUInt32();
  response.result = reader.ReadUInt32();
  response.detail_value = reader.ReadUInt32();
  reader.RequireEnd();
  return response;
}
}  // namespace bubble_fighter::server::protocol
