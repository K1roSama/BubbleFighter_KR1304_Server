#include "protocol/inventory/cash_inventory_pickup_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket CashInventoryPickupResponse::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(slot_id);
  writer.WriteUInt32(result);
  writer.WriteUInt32(value_at8);
  writer.WriteUtf16(text_at12);
  return {kMessageId, writer.TakeBytes()};
}
CashInventoryPickupResponse CashInventoryPickupResponse::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid cash pickup response message.");
  }
  PacketReader reader(packet.payload);
  CashInventoryPickupResponse result;
  result.slot_id = reader.ReadUInt32();
  result.result = reader.ReadUInt32();
  result.value_at8 = reader.ReadUInt32();
  result.text_at12 = reader.ReadUtf16();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
