#include "protocol/battle/game_actor_damage_result_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameActorDamageResultNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt32(source_object_id);
  for (auto bits : vector_bits) {
    writer.WriteUInt32(bits);
  }
  writer.WriteUInt32(weapon_id);
  writer.WriteUInt32(direct_death_value);
  writer.WriteUInt32(full_health_hit_value);
  writer.WriteUInt32(time_value);
  writer.WriteUInt32(value_at36);
  return {kMessageId, writer.TakeBytes()};
}

GameActorDamageResultNotification GameActorDamageResultNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid actor damage result notification.");
  }
  PacketReader reader(packet.payload);
  GameActorDamageResultNotification result;
  result.actor_object_id = reader.ReadUInt32();
  result.source_object_id = reader.ReadUInt32();
  for (auto& bits : result.vector_bits) {
    bits = reader.ReadUInt32();
  }
  result.weapon_id = reader.ReadUInt32();
  result.direct_death_value = reader.ReadUInt32();
  result.full_health_hit_value = reader.ReadUInt32();
  result.time_value = reader.ReadUInt32();
  result.value_at36 = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
