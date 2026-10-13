#include "protocol/battle/game_bubble_pop_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameBubblePopNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(apply_value);
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt32(source_object_id);
  for (auto value : vector_bits) {
    writer.WriteUInt32(value);
  }
  writer.WriteUInt32(contact_value);
  writer.WriteUInt32(reason_value);
  return {kMessageId, writer.TakeBytes()};
}

GameBubblePopNotification GameBubblePopNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid bubble message id.");
  }
  PacketReader reader(packet.payload);
  GameBubblePopNotification result;
  result.apply_value = reader.ReadUInt32();
  result.actor_object_id = reader.ReadUInt32();
  result.source_object_id = reader.ReadUInt32();
  for (auto& value : result.vector_bits) {
    value = reader.ReadUInt32();
  }
  result.contact_value = reader.ReadUInt32();
  result.reason_value = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
