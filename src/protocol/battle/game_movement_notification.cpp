#include "protocol/battle/game_movement_notification.h"

#include <bit>
#include <limits>
#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
static_assert(sizeof(float) == sizeof(std::uint32_t) &&
              std::numeric_limits<float>::is_iec559);

TcpPacket GameMovementNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  for (const auto value : position) {
    writer.WriteUInt32(std::bit_cast<std::uint32_t>(value));
  }
  writer.WriteUInt8(angle_at16);
  writer.WriteUInt8(angle_at17);
  writer.WriteUInt8(flags_at18);
  writer.WriteUInt16(value_at20);
  return {kMessageId, writer.TakeBytes()};
}

GameMovementNotification GameMovementNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid full movement message.");
  }
  PacketReader reader(packet.payload);
  GameMovementNotification result;
  result.actor_object_id = reader.ReadUInt32();
  for (auto& value : result.position) {
    value = std::bit_cast<float>(reader.ReadUInt32());
  }
  result.angle_at16 = reader.ReadUInt8();
  result.angle_at17 = reader.ReadUInt8();
  result.flags_at18 = reader.ReadUInt8();
  result.value_at20 = reader.ReadUInt16();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
