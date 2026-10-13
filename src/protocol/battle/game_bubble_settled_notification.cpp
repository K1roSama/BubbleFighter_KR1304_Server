#include "protocol/battle/game_bubble_settled_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameBubbleSettledNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  return {kMessageId, writer.TakeBytes()};
}
GameBubbleSettledNotification GameBubbleSettledNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid bubble settled message.");
  }
  PacketReader reader(packet.payload);
  GameBubbleSettledNotification result{reader.ReadUInt32()};
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
