#include "protocol/battle/game_start_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameStartNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(duration_milliseconds);
  writer.WriteUInt32(random_seed);
  return {kMessageId, writer.TakeBytes()};
}

GameStartNotification GameStartNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game start message.");
  }
  PacketReader reader(packet.payload);
  GameStartNotification result;
  result.duration_milliseconds = reader.ReadUInt32();
  result.random_seed = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
