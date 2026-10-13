#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_ITEM_USE_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_ITEM_USE_REQUEST_H_
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/inventory/item_use_arguments.h"

namespace bubble_fighter::server::protocol {
struct ItemUseRequest {
  static constexpr std::uint16_t kMessageId = 806;
  std::uint32_t item_id = 0;
  // Ordinary menu callers pass 1; meaning for other functions is unknown.
  std::uint32_t value_at4 = 0;
  std::vector<std::u16string> arguments;
  TcpPacket Encode() const;
  static ItemUseRequest Decode(const TcpPacket& packet);
  bool operator==(const ItemUseRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_ITEM_USE_REQUEST_H_
