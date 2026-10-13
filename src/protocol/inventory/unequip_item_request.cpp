#include "protocol/inventory/unequip_item_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket UnequipItemRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt64(item_identifier);
  return {kMessageId, writer.TakeBytes()};
}

UnequipItemRequest UnequipItemRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Incorrect equipment request message.");
  }
  PacketReader reader(packet.payload);
  UnequipItemRequest request{reader.ReadUInt64()};
  reader.RequireEnd();
  return request;
}
}  // namespace bubble_fighter::server::protocol
