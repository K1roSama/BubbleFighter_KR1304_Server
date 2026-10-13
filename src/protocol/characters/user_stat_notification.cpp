#include "protocol/characters/user_stat_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket UserStatNotification::Encode() const {
  PacketWriter writer;
  for (const auto value : values) {
    writer.WriteUInt32(value);
  }
  writer.WriteUtf16(text);
  writer.WriteUInt16(short_value);
  fighter.Write(writer);
  return {kMessageId, writer.TakeBytes()};
}

UserStatNotification UserStatNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument(
        "Unexpected user-stat notification message id.");
  }
  PacketReader reader(packet.payload);
  UserStatNotification message;
  for (auto& value : message.values) {
    value = reader.ReadUInt32();
  }
  message.text = reader.ReadUtf16();
  message.short_value = reader.ReadUInt16();
  message.fighter = FighterInfo::Read(reader);
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
