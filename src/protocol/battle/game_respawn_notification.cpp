#include "protocol/battle/game_respawn_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameRespawnNotification::Encode() const {
  PacketWriter writer;
  parameters.WriteTo(writer);
  writer.WriteUInt32(reset_items_value);
  writer.WriteUInt32(value_at24);
  return {kMessageId, writer.TakeBytes()};
}
GameRespawnNotification GameRespawnNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game respawn notification.");
  }
  PacketReader reader(packet.payload);
  GameRespawnNotification result;
  result.parameters = RespawnParameters::ReadFrom(reader);
  result.reset_items_value = reader.ReadUInt32();
  result.value_at24 = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
