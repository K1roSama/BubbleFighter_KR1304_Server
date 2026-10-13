#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_REFRESH_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_REFRESH_REQUEST_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E058: 0x00932E10 opens Inventory and sends C803 with no payload.
// 0x009A6D30 also refreshes an already open inventory after migration.
struct InventoryRefreshRequest {
  static constexpr std::uint16_t kMessageId = 0x0323;
  TcpPacket Encode() const;
  static InventoryRefreshRequest Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_REFRESH_REQUEST_H_
