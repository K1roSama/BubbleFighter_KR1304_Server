#include "protocol/rooms/room_list_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket RoomListRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt16(page);
  writer.WriteUInt16(page_size);
  return {kMessageId, writer.TakeBytes()};
}

RoomListRequest RoomListRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected room-list request id.");
  }
  PacketReader reader(packet.payload);
  RoomListRequest request;
  request.page = reader.ReadUInt16();
  request.page_size = reader.ReadUInt16();
  reader.RequireEnd();
  return request;
}
}  // namespace bubble_fighter::server::protocol
