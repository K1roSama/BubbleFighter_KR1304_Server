#include "protocol/battle/game_actor_respawn_wait_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameActorRespawnWaitNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  return {kMessageId, writer.TakeBytes()};
}

GameActorRespawnWaitNotification GameActorRespawnWaitNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid actor respawn wait message.");
  }
  PacketReader reader(packet.payload);
  GameActorRespawnWaitNotification result;
  result.actor_object_id = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
