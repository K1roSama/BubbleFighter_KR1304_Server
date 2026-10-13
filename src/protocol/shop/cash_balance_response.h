#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_CASH_BALANCE_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_CASH_BALANCE_RESPONSE_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct CashBalanceResponse {
  static constexpr std::uint16_t kMessageId = 834;
  // E065: both first and second fields are formatted as amounts in the
  // optional cash UI. Only the second is bound to UserStat.Cash. Neither
  // the first nor the third field is established as a result code.
  std::uint32_t value_at0 = 0;
  std::uint32_t cash = 0;
  std::uint32_t value_at8 = 0;

  TcpPacket Encode() const;
  static CashBalanceResponse Decode(const TcpPacket& packet);
  bool operator==(const CashBalanceResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_CASH_BALANCE_RESPONSE_H_
