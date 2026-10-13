#include "protocol/battle/game_bubble_contact_pop_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameBubbleContactPopRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt32(source_object_id);
  return {kMessageId, writer.TakeBytes()};
}

GameBubbleContactPopRequest GameBubbleContactPopRequest::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid bubble message id.");
  }
  PacketReader reader(packet.payload);
  GameBubbleContactPopRequest result;
  result.actor_object_id = reader.ReadUInt32();
  result.source_object_id = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
