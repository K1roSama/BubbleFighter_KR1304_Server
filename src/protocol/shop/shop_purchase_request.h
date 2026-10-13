#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_PURCHASE_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_PURCHASE_REQUEST_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct ShopPurchaseRequest {
  static constexpr std::uint16_t kMessageId = 809;
  std::uint32_t stock_id = 0;
  // 0x00E1D720 widens a native byte to uint32. Ordinary shop callers
  // pass zero; other values are not assigned gift/purchase semantics.
  std::uint32_t value_at4 = 0;
  TcpPacket Encode() const;
  static ShopPurchaseRequest Decode(const TcpPacket& packet);
  bool operator==(const ShopPurchaseRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_PURCHASE_REQUEST_H_
