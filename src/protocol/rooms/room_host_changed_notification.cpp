#include "protocol/rooms/room_host_changed_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket RoomHostChangedNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(host_slot);
  return {kMessageId, writer.TakeBytes()};
}

RoomHostChangedNotification RoomHostChangedNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected host-changed notification id.");
  }
  PacketReader reader(packet.payload);
  RoomHostChangedNotification message;
  message.host_slot = reader.ReadUInt32();
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
