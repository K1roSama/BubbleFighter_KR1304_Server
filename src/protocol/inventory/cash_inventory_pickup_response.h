#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_PICKUP_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_PICKUP_RESPONSE_H_

#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct CashInventoryPickupResponse {
  static constexpr std::uint16_t kMessageId = 835;
  static constexpr std::uint32_t kSuccess = 1;
  static constexpr std::uint32_t kFailed = 2;
  static constexpr std::uint32_t kAlreadyOwned = 8;
  std::uint32_t slot_id = 0;
  std::uint32_t result = 0;
  std::uint32_t value_at8 = 0;  // logged only when positive with nonempty text
  std::u16string text_at12;
  TcpPacket Encode() const;
  static CashInventoryPickupResponse Decode(const TcpPacket& packet);
  bool operator==(const CashInventoryPickupResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_PICKUP_RESPONSE_H_
