#include "protocol/battle/game_countdown_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameCountdownNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(duration_milliseconds);
  return {kMessageId, writer.TakeBytes()};
}

GameCountdownNotification GameCountdownNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game countdown message.");
  }
  PacketReader reader(packet.payload);
  GameCountdownNotification result;
  result.duration_milliseconds = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
