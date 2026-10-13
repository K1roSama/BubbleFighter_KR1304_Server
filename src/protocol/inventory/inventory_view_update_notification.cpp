#include "protocol/inventory/inventory_view_update_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket InventoryViewUpdateNotification::Encode() const {
  PacketWriter writer;
  std::size_t budget = UpdatePartyNotification::kMaxTotalEntries;
  view.Write(writer, budget);
  return {kMessageId, writer.TakeBytes()};
}

InventoryViewUpdateNotification InventoryViewUpdateNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected inventory-view update message id.");
  }
  PacketReader reader(packet.payload);
  std::size_t budget = UpdatePartyNotification::kMaxTotalEntries;
  InventoryViewUpdateNotification message;
  message.view = PartySupplementaryRecord::Read(reader, budget);
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
