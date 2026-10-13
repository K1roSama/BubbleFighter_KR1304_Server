#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_PICKUP_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_PICKUP_REQUEST_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct CashInventoryPickupRequest {
  static constexpr std::uint16_t kMessageId = 829;
  std::uint32_t slot_id = 0;
  TcpPacket Encode() const;
  static CashInventoryPickupRequest Decode(const TcpPacket& packet);
  bool operator==(const CashInventoryPickupRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_PICKUP_REQUEST_H_
