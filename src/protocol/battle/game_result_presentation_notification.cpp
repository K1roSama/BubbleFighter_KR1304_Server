#include "protocol/battle/game_result_presentation_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameResultPresentationNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt8(flags);
  return {kMessageId, writer.TakeBytes()};
}
GameResultPresentationNotification GameResultPresentationNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid result presentation message.");
  }
  PacketReader reader(packet.payload);
  GameResultPresentationNotification result{reader.ReadUInt8()};
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
