#include "protocol/battle/game_loading_complete_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameLoadingCompleteRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(delay_milliseconds);
  writer.WriteUInt32(value_at4);
  writer.WriteUInt32(value_at8);
  writer.WriteUInt32(value_at12);
  return {kMessageId, writer.TakeBytes()};
}

GameLoadingCompleteRequest GameLoadingCompleteRequest::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game loading completion message.");
  }
  PacketReader reader(packet.payload);
  GameLoadingCompleteRequest result;
  result.delay_milliseconds = reader.ReadUInt32();
  result.value_at4 = reader.ReadUInt32();
  result.value_at8 = reader.ReadUInt32();
  result.value_at12 = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
