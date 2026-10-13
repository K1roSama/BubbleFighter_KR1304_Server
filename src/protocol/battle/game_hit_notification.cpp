#include "protocol/battle/game_hit_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameHitNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(damager_key);
  writer.WriteUInt32(target_object_id);
  for (const auto value : position) {
    writer.WriteUInt16(value);
  }
  writer.WriteUInt8(hit_kind);
  writer.WriteUInt32(keep_damager_value);
  return {kMessageId, writer.TakeBytes()};
}

GameHitNotification GameHitNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid hit notification.");
  }
  PacketReader reader(packet.payload);
  GameHitNotification value;
  value.damager_key = reader.ReadUInt32();
  value.target_object_id = reader.ReadUInt32();
  for (auto& coordinate : value.position) {
    coordinate = reader.ReadUInt16();
  }
  value.hit_kind = reader.ReadUInt8();
  value.keep_damager_value = reader.ReadUInt32();
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
