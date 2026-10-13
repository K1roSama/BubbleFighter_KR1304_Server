#include "protocol/rooms/start_game_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket StartGameRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(variant);
  writer.WriteUtf16(map_resource);
  writer.WriteUtf16(map_attribute);
  writer.WriteUtf16(map_key);
  writer.WriteUInt32(value_at76);
  for (const auto value : rules.values) {
    writer.WriteUInt32(value);
  }
  writer.WriteUInt8(rules.byte_at52);
  writer.WriteUInt32(flags);
  return {kMessageId, writer.TakeBytes()};
}

StartGameRequest StartGameRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected start-game request id.");
  }
  PacketReader reader(packet.payload);
  StartGameRequest request;
  request.variant = reader.ReadUInt32();
  request.map_resource = reader.ReadUtf16();
  request.map_attribute = reader.ReadUtf16();
  request.map_key = reader.ReadUtf16();
  request.value_at76 = reader.ReadUInt32();
  for (auto& value : request.rules.values) {
    value = reader.ReadUInt32();
  }
  request.rules.byte_at52 = reader.ReadUInt8();
  request.flags = reader.ReadUInt32();
  reader.RequireEnd();
  if (request.variant > 0xFF || request.flags > 1) {
    throw std::runtime_error(
        "Start-game request values cannot come from the client writer.");
  }
  return request;
}
}  // namespace bubble_fighter::server::protocol
