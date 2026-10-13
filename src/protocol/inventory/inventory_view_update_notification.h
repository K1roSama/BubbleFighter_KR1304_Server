#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_VIEW_UPDATE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_VIEW_UPDATE_NOTIFICATION_H_

#include "protocol/scenes/update_party_notification.h"

namespace bubble_fighter::server::protocol {
// E049: S806 gspInventoryViewUpdateView, reader 0x009AF2F0.
// The leading key is compared with the local player identity. The rest
// uses the same supplementary record as S201/S117; meanings remain partial.
struct InventoryViewUpdateNotification {
  static constexpr std::uint16_t kMessageId = 0x0326;
  PartySupplementaryRecord view;

  TcpPacket Encode() const;
  static InventoryViewUpdateNotification Decode(const TcpPacket& packet);
  bool operator==(const InventoryViewUpdateNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_VIEW_UPDATE_NOTIFICATION_H_
