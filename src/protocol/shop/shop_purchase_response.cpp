#include "protocol/shop/shop_purchase_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket ShopPurchaseResponse::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(stock_id);
  writer.WriteUInt32(result);
  writer.WriteUtf16(text_at8);
  writer.WriteUInt8(byte_at32);
  writer.WriteUInt32(value_at36);
  writer.WriteUInt32(value_at40);
  writer.WriteUInt32(value_at44);
  return {kMessageId, writer.TakeBytes()};
}

ShopPurchaseResponse ShopPurchaseResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Incorrect shop purchase response message.");
  }
  PacketReader reader(packet.payload);
  ShopPurchaseResponse result;
  result.stock_id = reader.ReadUInt32();
  result.result = reader.ReadUInt32();
  result.text_at8 = reader.ReadUtf16();
  result.byte_at32 = reader.ReadUInt8();
  result.value_at36 = reader.ReadUInt32();
  result.value_at40 = reader.ReadUInt32();
  result.value_at44 = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
