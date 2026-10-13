#include "protocol/battle/game_extended_shot_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameExtendedShotNotification::Encode() const {
  if (values_at44.size() > kMaxValues) {
    throw std::length_error("Shot extension exceeds payload policy.");
  }
  PacketWriter writer;
  shot.Encode(writer);
  writer.WriteUInt32(value_at40);
  writer.WriteUInt32(static_cast<std::uint32_t>(values_at44.size()));
  for (const auto value : values_at44) {
    writer.WriteUInt32(value);
  }
  return {kMessageId, writer.TakeBytes()};
}

GameExtendedShotNotification GameExtendedShotNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid extended shot notification.");
  }
  if (packet.payload.size() > TcpPacketCodec::kDefaultMaxPayloadSize) {
    throw std::length_error("Shot extension exceeds payload policy.");
  }
  PacketReader reader(packet.payload);
  GameExtendedShotNotification result;
  result.shot = CompressedShotRecord::Decode(reader);
  result.value_at40 = reader.ReadUInt32();
  const auto count = reader.ReadUInt32();
  // Check the complete count before allocating, including extra bytes.
  if (count > kMaxValues || reader.RemainingBytes() % 4 != 0 ||
      count != reader.RemainingBytes() / 4) {
    throw std::runtime_error("Invalid shot extension count.");
  }
  result.values_at44.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    result.values_at44.push_back(reader.ReadUInt32());
  }
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
