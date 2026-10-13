#include "protocol/inventory/item_use_response.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
TcpPacket ItemUseResponse::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(result);
  writer.WriteUInt32(item_id);
  ItemUseArguments::Write(writer, arguments);
  return {kMessageId, writer.TakeBytes()};
}
ItemUseResponse ItemUseResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId ||
      packet.payload.size() > ItemUseArguments::kMaxPayloadBytes) {
    throw std::invalid_argument("Invalid item use response.");
  }
  PacketReader reader(packet.payload);
  ItemUseResponse value;
  value.result = reader.ReadUInt32();
  value.item_id = reader.ReadUInt32();
  value.arguments = ItemUseArguments::Read(reader);
  reader.RequireEnd();
  return value;
}
}  // namespace bubble_fighter::server::protocol
