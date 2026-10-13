#include "protocol/rooms/room_ready_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket RoomReadyRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(ready);
  writer.WriteUInt32(value_at4);
  writer.WriteUInt32(value_at8);
  return {kMessageId, writer.TakeBytes()};
}

RoomReadyRequest RoomReadyRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected room-ready request id.");
  }
  PacketReader reader(packet.payload);
  RoomReadyRequest request;
  request.ready = reader.ReadUInt32();
  request.value_at4 = reader.ReadUInt32();
  request.value_at8 = reader.ReadUInt32();
  reader.RequireEnd();
  // 0x009785E0 writes three zero-extended bytes. Its state-2 branch
  // re-tests 0x00626590 and can then copy a stale byte, so any byte
  // value is possible; consumers treat nonzero as set.
  if (request.ready > 0xFF || request.value_at4 > 0xFF ||
      request.value_at8 > 0xFF) {
    throw std::runtime_error(
        "Room-ready request values cannot come from the client writer.");
  }
  return request;
}
}  // namespace bubble_fighter::server::protocol
