#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_PURCHASE_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_PURCHASE_RESPONSE_H_

#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct ShopPurchaseResponse {
  static constexpr std::uint16_t kMessageId = 816;
  // Confirmed by 0x009B6730, not copied from the older server.
  static constexpr std::uint32_t kSuccess = 1;
  static constexpr std::uint32_t kInsufficientLucci = 0xFFFFFFF3U;  // -13
  static constexpr std::uint32_t kInsufficientCash = 36;
  static constexpr std::uint32_t kIneligible = 0xFFFFFF38U;  // -200
  static constexpr std::uint32_t kAlreadyOwned = 9;

  std::uint32_t stock_id = 0;
  // Preserve the exact bits, including negative error codes and unknowns.
  std::uint32_t result = 0;
  std::u16string text_at8;
  // A single byte, unlike the widened request byte. Nonzero values
  // select additional shop UI paths; meanings remain unresolved.
  std::uint8_t byte_at32 = 0;
  std::uint32_t value_at36 = 0;
  std::uint32_t value_at40 = 0;
  std::uint32_t value_at44 = 0;

  TcpPacket Encode() const;
  static ShopPurchaseResponse Decode(const TcpPacket& packet);
  bool operator==(const ShopPurchaseResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_PURCHASE_RESPONSE_H_
