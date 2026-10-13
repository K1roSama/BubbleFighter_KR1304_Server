#include "protocol/battle/game_bubble_deflect_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameBubbleDeflectNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt32(source_object_id);
  for (auto value : vector_bits) {
    writer.WriteUInt32(value);
  }
  writer.WriteUInt32(value_at20);
  writer.WriteUInt32(contact_value);
  return {kMessageId, writer.TakeBytes()};
}

GameBubbleDeflectNotification GameBubbleDeflectNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid bubble message id.");
  }
  PacketReader reader(packet.payload);
  GameBubbleDeflectNotification result;
  result.actor_object_id = reader.ReadUInt32();
  result.source_object_id = reader.ReadUInt32();
  for (auto& value : result.vector_bits) {
    value = reader.ReadUInt32();
  }
  result.value_at20 = reader.ReadUInt32();
  result.contact_value = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
