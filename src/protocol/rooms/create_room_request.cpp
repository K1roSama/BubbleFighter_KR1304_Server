#include "protocol/rooms/create_room_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket CreateRoomRequest::Encode() const {
  PacketWriter writer;
  settings.Write(writer);
  writer.WriteUtf16(password);
  writer.WriteUInt8(trailing_value ? kPresentMarker : kAbsentMarker);
  if (trailing_value) {
    writer.WriteUInt32(*trailing_value);
  }
  return {kMessageId, writer.TakeBytes()};
}

CreateRoomRequest CreateRoomRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected create-room request id.");
  }
  PacketReader reader(packet.payload);
  CreateRoomRequest request;
  request.settings = SessionDescriptor::Read(reader);
  request.password = reader.ReadUtf16();
  // 0x0096257A..0x00962585 only produces these two marker values.
  const auto marker = reader.ReadUInt8();
  if (marker == kPresentMarker) {
    request.trailing_value = reader.ReadUInt32();
  } else if (marker != kAbsentMarker) {
    throw std::runtime_error("Invalid create-room optional marker.");
  }
  reader.RequireEnd();
  return request;
}
}  // namespace bubble_fighter::server::protocol
