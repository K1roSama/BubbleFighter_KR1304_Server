#include "protocol/inventory/inventory_item_record.h"

#include <stdexcept>
#include <utility>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
namespace {
void CountEntries(std::size_t count, std::size_t& total) {
  if (count > InventoryItemRecord::kMaxCollectionEntries ||
      count > InventoryItemRecord::kMaxTotalCollectionEntries - total) {
    throw std::invalid_argument("Inventory collection exceeds local limits.");
  }
  total += count;
}

void CountBytes(std::size_t count, std::size_t& total) {
  if (count > InventoryItemRecord::kMaxUncompressedBytes - total) {
    throw std::invalid_argument("Inventory data exceeds local byte limit.");
  }
  total += count;
}

void CountText(std::u16string_view value, std::size_t& total) {
  if (value.size() > PacketWriter::kMaxStringCodeUnits) {
    throw std::invalid_argument("Inventory text exceeds local string limit.");
  }
  CountBytes(2 * value.size(), total);
}

std::uint32_t ReadCount(PacketReader& reader, std::size_t minimum_bytes,
                        std::size_t& total) {
  const auto count = reader.ReadUInt32();
  CountEntries(count, total);
  if (count > reader.RemainingBytes() / minimum_bytes) {
    throw std::invalid_argument("Truncated inventory collection.");
  }
  return count;
}
}  // namespace

std::vector<std::uint8_t> InventoryItemRecord::EncodeList(
    std::span<const InventoryItemRecord> records) {
  // Preflight the entire body before allocating a potentially large byte
  // vector. Fixed sizes include each string/vector count on the wire.
  std::size_t entries = 0;
  std::size_t bytes = 4;
  CountEntries(records.size(), entries);
  for (const auto& record : records) {
    CountBytes(25, bytes);
    CountText(record.text_at36, bytes);
    CountEntries(record.attributes_at24.size(), entries);
    for (const auto& attribute : record.attributes_at24) {
      CountBytes(20, bytes);
      CountText(attribute.text_at24, bytes);
      CountEntries(attribute.values_at8.size(), entries);
      CountBytes(4 * attribute.values_at8.size(), bytes);
    }
  }

  PacketWriter writer;
  writer.WriteUInt32(static_cast<std::uint32_t>(records.size()));
  for (const auto& record : records) {
    writer.WriteUInt32(record.key);
    writer.WriteUInt32(record.value_at4);
    writer.WriteUInt64(record.bits_at8);
    writer.WriteUtf16(record.text_at36);
    writer.WriteUInt32(
        static_cast<std::uint32_t>(record.attributes_at24.size()));
    for (const auto& attribute : record.attributes_at24) {
      writer.WriteUInt32(attribute.value_at0);
      writer.WriteUInt32(attribute.value_at4);
      writer.WriteUInt32(
          static_cast<std::uint32_t>(attribute.values_at8.size()));
      for (const auto value : attribute.values_at8) {
        writer.WriteUInt32(value);
      }
      writer.WriteUInt32(attribute.value_at20);
      writer.WriteUtf16(attribute.text_at24);
    }
    writer.WriteUInt8(record.flags);
  }
  return writer.TakeBytes();
}

std::vector<InventoryItemRecord> InventoryItemRecord::DecodeList(
    std::span<const std::uint8_t> bytes) {
  if (bytes.size() > kMaxUncompressedBytes) {
    throw std::invalid_argument("Inventory data exceeds local byte limit.");
  }
  PacketReader reader(bytes);
  std::size_t entries = 0;
  const auto count = ReadCount(reader, 25, entries);
  std::vector<InventoryItemRecord> records;
  records.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    InventoryItemRecord record;
    record.key = reader.ReadUInt32();
    record.value_at4 = reader.ReadUInt32();
    record.bits_at8 = reader.ReadUInt64();
    record.text_at36 = reader.ReadUtf16();
    const auto attribute_count = ReadCount(reader, 20, entries);
    record.attributes_at24.reserve(attribute_count);
    for (std::uint32_t attribute_index = 0; attribute_index < attribute_count;
         ++attribute_index) {
      InventoryAttributeRecord attribute;
      attribute.value_at0 = reader.ReadUInt32();
      attribute.value_at4 = reader.ReadUInt32();
      const auto value_count = ReadCount(reader, 4, entries);
      attribute.values_at8.reserve(value_count);
      for (std::uint32_t value_index = 0; value_index < value_count;
           ++value_index) {
        attribute.values_at8.push_back(reader.ReadUInt32());
      }
      attribute.value_at20 = reader.ReadUInt32();
      attribute.text_at24 = reader.ReadUtf16();
      record.attributes_at24.push_back(std::move(attribute));
    }
    record.flags = reader.ReadUInt8();
    records.push_back(std::move(record));
  }
  reader.RequireEnd();
  return records;
}
}  // namespace bubble_fighter::server::protocol
