#include "protocol/inventory/cash_inventory_update_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket CashInventoryUpdateNotification::Encode() const {
  if (slots.size() > kMaxSlots) {
    throw std::invalid_argument("Too many cash inventory slots.");
  }
  std::size_t bytes = 4;
  for (const auto& slot : slots) {
    if (slot.text_at24.size() > PacketWriter::kMaxStringCodeUnits ||
        25 + 2 * slot.text_at24.size() > kMaxPayloadBytes - bytes) {
      throw std::invalid_argument(
          "Cash inventory payload exceeds local limits.");
    }
    bytes += 25 + 2 * slot.text_at24.size();
  }
  PacketWriter writer;
  writer.WriteUInt32(static_cast<std::uint32_t>(slots.size()));
  for (const auto& slot : slots) {
    writer.WriteUInt32(slot.slot_id);
    writer.WriteUInt32(slot.value_at8);
    writer.WriteUInt16(slot.quantity);
    writer.WriteUInt16(slot.duration_days);
    writer.WriteUInt32(slot.stock_id);
    writer.WriteUInt32(slot.item_id);
    writer.WriteUtf16(slot.text_at24);
    writer.WriteUInt8(slot.byte_at48);
  }
  return {kMessageId, writer.TakeBytes()};
}

CashInventoryUpdateNotification CashInventoryUpdateNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId ||
      packet.payload.size() > kMaxPayloadBytes) {
    throw std::invalid_argument("Invalid cash inventory message or size.");
  }
  PacketReader reader(packet.payload);
  const auto count = reader.ReadUInt32();
  if (count > kMaxSlots || count > reader.RemainingBytes() / 25) {
    throw std::invalid_argument("Invalid cash inventory slot count.");
  }
  CashInventoryUpdateNotification result;
  result.slots.reserve(count);
  for (std::uint32_t i = 0; i != count; ++i) {
    CashInventorySlotRecord slot;
    slot.slot_id = reader.ReadUInt32();
    slot.value_at8 = reader.ReadUInt32();
    slot.quantity = reader.ReadUInt16();
    slot.duration_days = reader.ReadUInt16();
    slot.stock_id = reader.ReadUInt32();
    slot.item_id = reader.ReadUInt32();
    slot.text_at24 = reader.ReadUtf16();
    slot.byte_at48 = reader.ReadUInt8();
    result.slots.push_back(std::move(slot));
  }
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
