#include "protocol/rooms/room_slot_update_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket RoomSlotUpdateNotification::Encode() const {
  PacketWriter writer;
  std::size_t budget = GotoRoomNotification::kMaxTotalCollectionEntries;
  writer.WriteUInt32(slot);
  WriteSessionMember(writer, member, budget);
  WriteSessionRecords(writer, records_at564, budget);
  WriteSessionRecords(writer, records_at588, budget);
  WriteSessionTeam(writer, team, budget);
  return {kMessageId, writer.TakeBytes()};
}

RoomSlotUpdateNotification RoomSlotUpdateNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected slot-update notification id.");
  }
  PacketReader reader(packet.payload);
  std::size_t budget = GotoRoomNotification::kMaxTotalCollectionEntries;
  RoomSlotUpdateNotification message;
  message.slot = reader.ReadUInt32();
  message.member = ReadSessionMember(reader, budget);
  message.records_at564 = ReadSessionRecords(reader, budget);
  message.records_at588 = ReadSessionRecords(reader, budget);
  message.team = ReadSessionTeam(reader, budget);
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
