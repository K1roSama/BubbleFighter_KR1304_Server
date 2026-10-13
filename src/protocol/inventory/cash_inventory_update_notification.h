#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_UPDATE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_UPDATE_NOTIFICATION_H_

#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/inventory/cash_inventory_slot_record.h"

namespace bubble_fighter::server::protocol {
struct CashInventoryUpdateNotification {
  static constexpr std::uint16_t kMessageId = 810;
  // Local allocation policies, not recovered original-server limits.
  static constexpr std::size_t kMaxSlots = 1024;
  static constexpr std::size_t kMaxPayloadBytes = 262144;
  std::vector<CashInventorySlotRecord> slots;
  TcpPacket Encode() const;
  static CashInventoryUpdateNotification Decode(const TcpPacket& packet);
  bool operator==(const CashInventoryUpdateNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_UPDATE_NOTIFICATION_H_
