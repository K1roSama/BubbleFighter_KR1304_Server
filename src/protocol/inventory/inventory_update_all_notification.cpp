#include "protocol/inventory/inventory_update_all_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
InventoryUpdateAllNotification InventoryUpdateAllNotification::FromRecords(
    std::span<const InventoryItemRecord> records) {
  return {ZlibStoredStream::Encode(InventoryItemRecord::EncodeList(records))};
}

TcpPacket InventoryUpdateAllNotification::Encode() const {
  if (compressed_records.empty() ||
      compressed_records.size() > kMaxCompressedBytes) {
    throw std::invalid_argument("Invalid compressed inventory size.");
  }
  PacketWriter writer;
  writer.WriteUInt32(static_cast<std::uint32_t>(compressed_records.size()));
  for (const auto value : compressed_records) {
    writer.WriteUInt8(value);
  }
  return {kMessageId, writer.TakeBytes()};
}

InventoryUpdateAllNotification InventoryUpdateAllNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected inventory notification id.");
  }
  PacketReader reader(packet.payload);
  const auto count = reader.ReadUInt32();
  if (count == 0 || count > kMaxCompressedBytes ||
      count != reader.RemainingBytes()) {
    throw std::invalid_argument("Invalid compressed inventory length.");
  }
  InventoryUpdateAllNotification result;
  result.compressed_records.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    result.compressed_records.push_back(reader.ReadUInt8());
  }
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
