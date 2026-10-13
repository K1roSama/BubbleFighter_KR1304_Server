#include "protocol/battle/game_grenade_detonate_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameGrenadeDetonateNotification::Encode() const {
  if (values.size() > kMaxValues) {
    throw std::length_error("Grenade extension exceeds payload policy.");
  }
  PacketWriter writer;
  writer.WriteUInt32(source_damager_key);
  for (const auto value : position_bits) {
    writer.WriteUInt32(value);
  }
  for (const auto value : vector_bits) {
    writer.WriteUInt32(value);
  }
  writer.WriteUInt32(damager_key);
  writer.WriteUInt32(packed_time);
  writer.WriteUInt32(static_cast<std::uint32_t>(values.size()));
  for (const auto value : values) {
    writer.WriteUInt32(value);
  }
  return {kMessageId, writer.TakeBytes()};
}

GameGrenadeDetonateNotification GameGrenadeDetonateNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid grenade detonation message.");
  }
  if (packet.payload.size() > TcpPacketCodec::kDefaultMaxPayloadSize) {
    throw std::length_error("Grenade extension exceeds payload policy.");
  }
  PacketReader reader(packet.payload);
  GameGrenadeDetonateNotification value;
  value.source_damager_key = reader.ReadUInt32();
  for (auto& coordinate : value.position_bits) {
    coordinate = reader.ReadUInt32();
  }
  for (auto& coordinate : value.vector_bits) {
    coordinate = reader.ReadUInt32();
  }
  value.damager_key = reader.ReadUInt32();
  value.packed_time = reader.ReadUInt32();
  const auto count = reader.ReadUInt32();
  if (count > kMaxValues || reader.RemainingBytes() % 4 != 0 ||
      count != reader.RemainingBytes() / 4) {
    throw std::runtime_error("Invalid grenade extension count.");
  }
  value.values.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    value.values.push_back(reader.ReadUInt32());
  }
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
