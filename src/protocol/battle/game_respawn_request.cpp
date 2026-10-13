#include "protocol/battle/game_respawn_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameRespawnRequest::Encode() const {
  PacketWriter writer;
  parameters.WriteTo(writer);
  writer.WriteUInt32(reset_items_value);
  return {kMessageId, writer.TakeBytes()};
}
GameRespawnRequest GameRespawnRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game respawn request.");
  }
  PacketReader reader(packet.payload);
  GameRespawnRequest result;
  result.parameters = RespawnParameters::ReadFrom(reader);
  result.reset_items_value = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
