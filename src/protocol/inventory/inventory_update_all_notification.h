#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_UPDATE_ALL_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_UPDATE_ALL_NOTIFICATION_H_

#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/inventory/inventory_item_record.h"

namespace bubble_fighter::server::protocol {
// gspInventoryUpdateAllInventorySlot, E052. Framing is a uint32 compressed
// length followed by a zlib stream, not a bare inventory record vector.
struct InventoryUpdateAllNotification {
  static constexpr std::uint16_t kMessageId = 0x0322;
  // Allows all stored-block output at the existing 256 KiB input limit.
  static constexpr std::size_t kMaxCompressedBytes =
      ZlibStoredStream::kMaxInputBytes + 64;

  std::vector<std::uint8_t> compressed_records;

  static InventoryUpdateAllNotification FromRecords(
      std::span<const InventoryItemRecord> records);
  TcpPacket Encode() const;
  // Decodes framing only, preserving arbitrary zlib data. Does not inflate
  // or validate compressed contents. FromRecords creates server output.
  static InventoryUpdateAllNotification Decode(const TcpPacket& packet);
  bool operator==(const InventoryUpdateAllNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_UPDATE_ALL_NOTIFICATION_H_
