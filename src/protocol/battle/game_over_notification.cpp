#include "protocol/battle/game_over_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameOverNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(reason);
  writer.WriteUInt32(value_at4);
  return {kMessageId, writer.TakeBytes()};
}
GameOverNotification GameOverNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game-over message.");
  }
  PacketReader reader(packet.payload);
  GameOverNotification result{reader.ReadUInt32(), reader.ReadUInt32()};
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
