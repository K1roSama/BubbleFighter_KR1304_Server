#include "protocol/battle/game_result_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameResultNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(value_at0);
  writer.WriteUInt32(elapsed_milliseconds);
  writer.WriteUInt8(byte_at8);
  writer.WriteUInt8(byte_at9);
  return {kMessageId, writer.TakeBytes()};
}
GameResultNotification GameResultNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game result message.");
  }
  PacketReader reader(packet.payload);
  GameResultNotification result{reader.ReadUInt32(), reader.ReadUInt32(),
                                reader.ReadUInt8(), reader.ReadUInt8()};
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
