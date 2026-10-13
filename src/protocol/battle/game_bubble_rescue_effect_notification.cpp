#include "protocol/battle/game_bubble_rescue_effect_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameBubbleRescueEffectNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(helper_object_id);
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt8(kind);
  return {kMessageId, writer.TakeBytes()};
}

GameBubbleRescueEffectNotification GameBubbleRescueEffectNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid bubble message id.");
  }
  PacketReader reader(packet.payload);
  GameBubbleRescueEffectNotification result;
  result.helper_object_id = reader.ReadUInt32();
  result.actor_object_id = reader.ReadUInt32();
  result.kind = reader.ReadUInt8();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
