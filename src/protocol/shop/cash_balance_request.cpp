#include "protocol/shop/cash_balance_request.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
TcpPacket CashBalanceRequest::Encode() const { return {kMessageId, {}}; }

CashBalanceRequest CashBalanceRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId || !packet.payload.empty()) {
    throw std::invalid_argument("Invalid empty cash balance request.");
  }
  return {};
}
}  // namespace bubble_fighter::server::protocol
