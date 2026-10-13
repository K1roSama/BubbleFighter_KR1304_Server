#include "protocol/rooms/join_room_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket JoinRoomResponse::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(result);
  writer.WriteUInt32(detail_value);
  return {kMessageId, writer.TakeBytes()};
}

JoinRoomResponse JoinRoomResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected join-room response id.");
  }
  PacketReader reader(packet.payload);
  JoinRoomResponse response;
  response.result = reader.ReadUInt32();
  response.detail_value = reader.ReadUInt32();
  reader.RequireEnd();
  return response;
}
}  // namespace bubble_fighter::server::protocol
