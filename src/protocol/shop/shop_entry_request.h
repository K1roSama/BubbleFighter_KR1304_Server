#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_ENTRY_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_ENTRY_REQUEST_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E061: 0x009328B0 writes the cached catalog revision and category.
struct ShopEntryRequest {
  static constexpr std::uint16_t kMessageId = 801;
  std::uint64_t catalog_revision = 0;
  std::uint32_t selected_category = 0;
  TcpPacket Encode() const;
  static ShopEntryRequest Decode(const TcpPacket& packet);
  bool operator==(const ShopEntryRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_ENTRY_REQUEST_H_
