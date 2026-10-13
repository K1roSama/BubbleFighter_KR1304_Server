#include "protocol/battle/game_bubble_action_end_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameBubbleActionEndNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  return {kMessageId, writer.TakeBytes()};
}
GameBubbleActionEndNotification GameBubbleActionEndNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid bubble action end id.");
  }
  PacketReader reader(packet.payload);
  GameBubbleActionEndNotification value{reader.ReadUInt32()};
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
