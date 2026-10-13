#include "protocol/battle/game_respawn_completion_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameRespawnCompletionNotification::Encode() const {
  PacketWriter writer;
  parameters.WriteTo(writer);
  writer.WriteUInt8(byte_at20);
  writer.WriteUInt32(value_at21);
  return {kMessageId, writer.TakeBytes()};
}
GameRespawnCompletionNotification GameRespawnCompletionNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid respawn completion notification.");
  }
  PacketReader reader(packet.payload);
  GameRespawnCompletionNotification result;
  result.parameters = RespawnParameters::ReadFrom(reader);
  result.byte_at20 = reader.ReadUInt8();
  result.value_at21 = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
