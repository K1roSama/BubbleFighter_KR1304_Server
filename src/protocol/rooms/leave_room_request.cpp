#include "protocol/rooms/leave_room_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"

namespace bubble_fighter::server::protocol {
TcpPacket LeaveRoomRequest::Encode() const { return {kMessageId, {}}; }

LeaveRoomRequest LeaveRoomRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected leave-room request id.");
  }
  PacketReader(packet.payload).RequireEnd();
  return {};
}
}  // namespace bubble_fighter::server::protocol
