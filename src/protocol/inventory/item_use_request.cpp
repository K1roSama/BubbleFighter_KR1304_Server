#include "protocol/inventory/item_use_request.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
TcpPacket ItemUseRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(item_id);
  writer.WriteUInt32(value_at4);
  ItemUseArguments::Write(writer, arguments);
  return {kMessageId, writer.TakeBytes()};
}
ItemUseRequest ItemUseRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId ||
      packet.payload.size() > ItemUseArguments::kMaxPayloadBytes) {
    throw std::invalid_argument("Invalid item use request.");
  }
  PacketReader reader(packet.payload);
  ItemUseRequest value;
  value.item_id = reader.ReadUInt32();
  value.value_at4 = reader.ReadUInt32();
  value.arguments = ItemUseArguments::Read(reader);
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
