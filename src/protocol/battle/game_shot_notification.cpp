#include "protocol/battle/game_shot_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameShotNotification::Encode() const {
  PacketWriter writer;
  shot.Encode(writer);
  return {kMessageId, writer.TakeBytes()};
}

GameShotNotification GameShotNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid shot notification.");
  }
  PacketReader reader(packet.payload);
  GameShotNotification result{CompressedShotRecord::Decode(reader)};
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
