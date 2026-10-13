#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_CASH_BALANCE_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_CASH_BALANCE_REQUEST_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct CashBalanceRequest {
  static constexpr std::uint16_t kMessageId = 828;
  TcpPacket Encode() const;
  static CashBalanceRequest Decode(const TcpPacket& packet);
  bool operator==(const CashBalanceRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_CASH_BALANCE_REQUEST_H_
