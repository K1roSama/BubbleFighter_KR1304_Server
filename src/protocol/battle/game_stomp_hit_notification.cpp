#include "protocol/battle/game_stomp_hit_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameStompHitNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt32(target_object_id);
  return {kMessageId, writer.TakeBytes()};
}
GameStompHitNotification GameStompHitNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid bubble stomp hit.");
  }
  PacketReader reader(packet.payload);
  GameStompHitNotification value;
  value.actor_object_id = reader.ReadUInt32();
  value.target_object_id = reader.ReadUInt32();
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
