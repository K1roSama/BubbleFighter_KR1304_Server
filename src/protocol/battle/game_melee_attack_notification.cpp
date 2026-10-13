#include "protocol/battle/game_melee_attack_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameMeleeAttackNotification::Encode() const {
  if (values.size() > kMaxValues) {
    throw std::length_error("Melee extension exceeds payload policy.");
  }
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt16(weapon_id);
  writer.WriteUInt8(byte_at6);
  writer.WriteUInt32(shot_sequence);
  writer.WriteUInt32(packed_time);
  writer.WriteUInt32(static_cast<std::uint32_t>(values.size()));
  for (const auto value : values) {
    writer.WriteUInt32(value);
  }
  return {kMessageId, writer.TakeBytes()};
}

GameMeleeAttackNotification GameMeleeAttackNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid melee attack message.");
  }
  if (packet.payload.size() > TcpPacketCodec::kDefaultMaxPayloadSize) {
    throw std::length_error("Melee extension exceeds payload policy.");
  }
  PacketReader reader(packet.payload);
  GameMeleeAttackNotification result;
  result.actor_object_id = reader.ReadUInt32();
  result.weapon_id = reader.ReadUInt16();
  result.byte_at6 = reader.ReadUInt8();
  result.shot_sequence = reader.ReadUInt32();
  result.packed_time = reader.ReadUInt32();
  const auto count = reader.ReadUInt32();
  if (count > kMaxValues || reader.RemainingBytes() % 4 != 0 ||
      count != reader.RemainingBytes() / 4) {
    throw std::runtime_error("Invalid melee extension count.");
  }
  result.values.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    result.values.push_back(reader.ReadUInt32());
  }
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
