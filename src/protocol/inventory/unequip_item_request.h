#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_UNEQUIP_ITEM_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_UNEQUIP_ITEM_REQUEST_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct UnequipItemRequest {
  static constexpr std::uint16_t kMessageId = 805;
  // Current client writes the full 64-bit identifier. Ordinary item IDs
  // and ArticleSlot instances are separated by the business layer.
  std::uint64_t item_identifier = 0;
  TcpPacket Encode() const;
  static UnequipItemRequest Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_UNEQUIP_ITEM_REQUEST_H_
