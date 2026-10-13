#include "protocol/battle/game_actor_death_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameActorDeathNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  return {kMessageId, writer.TakeBytes()};
}

GameActorDeathNotification GameActorDeathNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid actor death message.");
  }
  PacketReader reader(packet.payload);
  GameActorDeathNotification result;
  result.actor_object_id = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
