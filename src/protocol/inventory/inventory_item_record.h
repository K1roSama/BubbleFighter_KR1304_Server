#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_ITEM_RECORD_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_ITEM_RECORD_H_

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "protocol/codec/zlib_stored_stream.h"

namespace bubble_fighter::server::protocol {
struct InventoryAttributeRecord {
  // E052: 0x010D9410. Offsets are in the client object, not the wire.
  std::uint32_t value_at0 = 0;
  std::uint32_t value_at4 = 0;
  std::vector<std::uint32_t> values_at8;
  std::uint32_t value_at20 = 0;
  std::u16string text_at24;
  bool operator==(const InventoryAttributeRecord&) const = default;
};

struct InventoryItemRecord {
  // E052: S802 decompresses before 0x010D8550 reads this collection.
  // E053 identifies Key as item ID, ValueAt4 as count/Life, BitsAt8 as
  // remaining days (IEEE double), and bits 0/1 as equipped/exhausted.
  // Keep raw names here for lossless wire decoding; the Inventory feature
  // validates ordinary slots and computes duration from stored expiry.
  static constexpr std::uint32_t kMaxCollectionEntries = 1024;
  static constexpr std::uint32_t kMaxTotalCollectionEntries = 4096;
  static constexpr std::size_t kMaxUncompressedBytes =
      ZlibStoredStream::kMaxInputBytes;

  std::uint32_t key = 0;
  std::uint32_t value_at4 = 0;
  std::uint64_t bits_at8 = 0;
  std::u16string text_at36;
  std::vector<InventoryAttributeRecord> attributes_at24;
  // Wire bits map to client bytes +16,+17,+18,+19,+20,+22,+21,+23.
  std::uint8_t flags = 0;

  static std::vector<std::uint8_t> EncodeList(
      std::span<const InventoryItemRecord> records);
  static std::vector<InventoryItemRecord> DecodeList(
      std::span<const std::uint8_t> bytes);
  bool operator==(const InventoryItemRecord&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_INVENTORY_ITEM_RECORD_H_
