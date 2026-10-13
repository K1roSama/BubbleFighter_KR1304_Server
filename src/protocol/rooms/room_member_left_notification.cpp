#include "protocol/rooms/room_member_left_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket RoomMemberLeftNotification::Encode() const {
  PacketWriter writer;
  std::size_t budget = GotoRoomNotification::kMaxTotalCollectionEntries;
  writer.WriteUInt32(slot);
  WriteSessionMember(writer, member, budget);
  WriteSessionTeam(writer, team, budget);
  return {kMessageId, writer.TakeBytes()};
}

RoomMemberLeftNotification RoomMemberLeftNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected member-left notification id.");
  }
  PacketReader reader(packet.payload);
  std::size_t budget = GotoRoomNotification::kMaxTotalCollectionEntries;
  RoomMemberLeftNotification message;
  message.slot = reader.ReadUInt32();
  message.member = ReadSessionMember(reader, budget);
  message.team = ReadSessionTeam(reader, budget);
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
