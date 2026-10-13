#include "protocol/battle/game_weapon_impact_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameWeaponImpactNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(damager_key);
  for (const auto value : position_bits) {
    writer.WriteUInt32(value);
  }
  writer.WriteUInt32(source_object_id);
  writer.WriteUInt32(target_object_id);
  return {kMessageId, writer.TakeBytes()};
}

GameWeaponImpactNotification GameWeaponImpactNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid weapon impact message.");
  }
  PacketReader reader(packet.payload);
  GameWeaponImpactNotification value;
  value.damager_key = reader.ReadUInt32();
  for (auto& coordinate : value.position_bits) {
    coordinate = reader.ReadUInt32();
  }
  value.source_object_id = reader.ReadUInt32();
  value.target_object_id = reader.ReadUInt32();
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
