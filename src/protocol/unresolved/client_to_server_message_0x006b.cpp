#include "protocol/unresolved/client_to_server_message_0x006b.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ClientToServerMessage0x006B::Encode() const {
  constexpr std::size_t fixed_size = 13 * 4 + 1 + 3 * 4 + 4;
  std::size_t size = fixed_size;
  for (const auto* text : {&directory, &map_name, &map_key}) {
    if (text->size() > PacketWriter::kMaxStringCodeUnits ||
        text->size() > (kMaxPayloadBytes - size) / 2) {
      throw std::length_error("C107 exceeds local string or payload limit.");
    }
    size += text->size() * 2;
  }
  PacketWriter writer;
  for (const auto value : rules) {
    writer.WriteUInt32(value);
  }
  writer.WriteUInt8(rule_flags);
  writer.WriteUtf16(directory);
  writer.WriteUtf16(map_name);
  writer.WriteUtf16(map_key);
  writer.WriteUInt32(difficulty);
  return {kMessageId, writer.TakeBytes()};
}

ClientToServerMessage0x006B ClientToServerMessage0x006B::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid C107 message.");
  }
  if (packet.payload.size() > kMaxPayloadBytes) {
    throw std::length_error("C107 exceeds local payload limit.");
  }
  PacketReader reader(packet.payload);
  ClientToServerMessage0x006B result;
  for (auto& value : result.rules) {
    value = reader.ReadUInt32();
  }
  result.rule_flags = reader.ReadUInt8();
  result.directory = reader.ReadUtf16();
  result.map_name = reader.ReadUtf16();
  result.map_key = reader.ReadUtf16();
  result.difficulty = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
