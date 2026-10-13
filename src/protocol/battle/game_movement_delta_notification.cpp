#include "protocol/battle/game_movement_delta_notification.h"

#include <bit>
#include <limits>
#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
static_assert(sizeof(float) == sizeof(std::uint32_t) &&
              std::numeric_limits<float>::is_iec559);

TcpPacket GameMovementDeltaNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt16(value_at4);
  writer.WriteUInt8(mask);
  for (std::size_t i = 0; i < position.size(); ++i) {
    if (((mask & (1U << i)) != 0) != position[i].has_value()) {
      throw std::invalid_argument(
          "Movement delta position does not match its mask.");
    }
    if (position[i]) {
      writer.WriteUInt32(std::bit_cast<std::uint32_t>(*position[i]));
    }
  }
  for (std::size_t i = 0; i < angles_and_flags.size(); ++i) {
    if (((mask & (8U << i)) != 0) != angles_and_flags[i].has_value()) {
      throw std::invalid_argument(
          "Movement delta byte does not match its mask.");
    }
    if (angles_and_flags[i]) {
      writer.WriteUInt8(*angles_and_flags[i]);
    }
  }
  return {kMessageId, writer.TakeBytes()};
}

GameMovementDeltaNotification GameMovementDeltaNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid movement delta message.");
  }
  PacketReader reader(packet.payload);
  GameMovementDeltaNotification result;
  result.actor_object_id = reader.ReadUInt32();
  result.value_at4 = reader.ReadUInt16();
  result.mask = reader.ReadUInt8();
  for (std::size_t i = 0; i < result.position.size(); ++i) {
    if ((result.mask & (1U << i)) != 0) {
      result.position[i] = std::bit_cast<float>(reader.ReadUInt32());
    }
  }
  for (std::size_t i = 0; i < result.angles_and_flags.size(); ++i) {
    if ((result.mask & (8U << i)) != 0) {
      result.angles_and_flags[i] = reader.ReadUInt8();
    }
  }
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
