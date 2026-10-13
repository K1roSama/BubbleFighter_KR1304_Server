#include "protocol/shop/shop_entry_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ShopEntryRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt64(catalog_revision);
  writer.WriteUInt32(selected_category);
  return {kMessageId, writer.TakeBytes()};
}

ShopEntryRequest ShopEntryRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Incorrect shop entry request message.");
  }
  PacketReader reader(packet.payload);
  ShopEntryRequest request;
  request.catalog_revision = reader.ReadUInt64();
  request.selected_category = reader.ReadUInt32();
  reader.RequireEnd();
  return request;
}
}  // namespace bubble_fighter::server::protocol
