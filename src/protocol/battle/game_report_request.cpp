#include "protocol/battle/game_report_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameReportRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(random_seed);
  for (const auto value : values_at4) {
    writer.WriteUInt32(value);
  }
  return {kMessageId, writer.TakeBytes()};
}
GameReportRequest GameReportRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game report message.");
  }
  PacketReader reader(packet.payload);
  GameReportRequest result;
  result.random_seed = reader.ReadUInt32();
  for (auto& value : result.values_at4) {
    value = reader.ReadUInt32();
  }
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
