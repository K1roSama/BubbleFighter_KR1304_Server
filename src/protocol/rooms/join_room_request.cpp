#include "protocol/rooms/join_room_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket JoinRoomRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(room_id);
  writer.WriteUtf16(password);
  return {kMessageId, writer.TakeBytes()};
}

JoinRoomRequest JoinRoomRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected join-room request id.");
  }
  PacketReader reader(packet.payload);
  JoinRoomRequest request;
  request.room_id = reader.ReadUInt32();
  request.password = reader.ReadUtf16();
  reader.RequireEnd();
  return request;
}
}  // namespace bubble_fighter::server::protocol
