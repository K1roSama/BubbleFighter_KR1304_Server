#include "protocol/battle/game_bubble_action_start_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameBubbleActionStartNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt32(target_object_id);
  writer.WriteUInt32(variant);
  return {kMessageId, writer.TakeBytes()};
}
GameBubbleActionStartNotification GameBubbleActionStartNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid bubble action start id.");
  }
  PacketReader reader(packet.payload);
  GameBubbleActionStartNotification value;
  value.actor_object_id = reader.ReadUInt32();
  value.target_object_id = reader.ReadUInt32();
  value.variant = reader.ReadUInt32();
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
