#include "protocol/battle/game_damager_remove_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameDamagerRemoveNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(damager_key);
  return {kMessageId, writer.TakeBytes()};
}

GameDamagerRemoveNotification GameDamagerRemoveNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid damager removal notification.");
  }
  PacketReader reader(packet.payload);
  GameDamagerRemoveNotification value{reader.ReadUInt32()};
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
