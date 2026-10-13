#include "protocol/inventory/cash_inventory_pickup_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket CashInventoryPickupRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(slot_id);
  return {kMessageId, writer.TakeBytes()};
}
CashInventoryPickupRequest CashInventoryPickupRequest::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid cash pickup request message.");
  }
  PacketReader reader(packet.payload);
  CashInventoryPickupRequest result{reader.ReadUInt32()};
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
