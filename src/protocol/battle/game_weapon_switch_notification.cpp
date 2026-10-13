#include "protocol/battle/game_weapon_switch_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameWeaponSwitchNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(weapon_id);
  writer.WriteUInt32(weapon_index);
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt32(time_value);
  writer.WriteUInt32(select_value);
  writer.WriteUInt32(value_at20);
  return {kMessageId, writer.TakeBytes()};
}
GameWeaponSwitchNotification GameWeaponSwitchNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid weapon switch message.");
  }
  PacketReader reader(packet.payload);
  GameWeaponSwitchNotification result;
  result.weapon_id = reader.ReadUInt32();
  result.weapon_index = reader.ReadUInt32();
  result.actor_object_id = reader.ReadUInt32();
  result.time_value = reader.ReadUInt32();
  result.select_value = reader.ReadUInt32();
  result.value_at20 = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
