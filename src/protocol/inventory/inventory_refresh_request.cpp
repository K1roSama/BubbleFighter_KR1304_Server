#include "protocol/inventory/inventory_refresh_request.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
TcpPacket InventoryRefreshRequest::Encode() const { return {kMessageId, {}}; }

InventoryRefreshRequest InventoryRefreshRequest::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId || !packet.payload.empty()) {
    throw std::invalid_argument("Invalid empty inventory refresh request.");
  }
  return {};
}
}  // namespace bubble_fighter::server::protocol
