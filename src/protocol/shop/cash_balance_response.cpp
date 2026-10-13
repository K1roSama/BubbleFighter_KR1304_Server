#include "protocol/shop/cash_balance_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket CashBalanceResponse::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(value_at0);
  writer.WriteUInt32(cash);
  writer.WriteUInt32(value_at8);
  return {kMessageId, writer.TakeBytes()};
}

CashBalanceResponse CashBalanceResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid cash balance response message.");
  }
  PacketReader reader(packet.payload);
  CashBalanceResponse result;
  result.value_at0 = reader.ReadUInt32();
  result.cash = reader.ReadUInt32();
  result.value_at8 = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
