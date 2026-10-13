#include "protocol/shop/shop_purchase_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ShopPurchaseRequest::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(stock_id);
  writer.WriteUInt32(value_at4);
  return {kMessageId, writer.TakeBytes()};
}

ShopPurchaseRequest ShopPurchaseRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Incorrect shop purchase request message.");
  }
  PacketReader reader(packet.payload);
  ShopPurchaseRequest result;
  result.stock_id = reader.ReadUInt32();
  result.value_at4 = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
