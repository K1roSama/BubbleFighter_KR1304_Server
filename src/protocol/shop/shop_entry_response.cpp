#include "protocol/shop/shop_entry_response.h"

#include <stdexcept>
#include <utility>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
namespace {
constexpr std::size_t kPriceBytes = 26;
constexpr std::size_t kItemMinimumBytes = 74;
constexpr std::size_t kCategoryMinimumBytes = 16;

void CountEntries(std::size_t count, std::size_t& total) {
  if (count > ShopEntryResponse::kMaxCollectionEntries ||
      count > ShopEntryResponse::kMaxTotalCollectionEntries - total) {
    throw std::invalid_argument("Shop collection exceeds local limits.");
  }
  total += count;
}

void CountBytes(std::size_t count, std::size_t& total) {
  if (count > ShopEntryResponse::kMaxPayloadBytes - total) {
    throw std::invalid_argument("Shop response exceeds local byte limit.");
  }
  total += count;
}

void CountText(std::u16string_view text, std::size_t& total) {
  if (text.size() > PacketWriter::kMaxStringCodeUnits) {
    throw std::invalid_argument("Shop text exceeds local string limit.");
  }
  CountBytes(2 * text.size(), total);
}

std::uint32_t ReadCount(PacketReader& reader, std::size_t minimum_bytes,
                        std::size_t& entries) {
  const auto count = reader.ReadUInt32();
  CountEntries(count, entries);
  if (count > reader.RemainingBytes() / minimum_bytes) {
    throw std::invalid_argument("Truncated shop collection.");
  }
  return count;
}

void WritePrice(PacketWriter& writer, const ShopPriceRecord& price) {
  writer.WriteUInt32(price.stock_id);
  writer.WriteUInt32(price.item_id);
  writer.WriteUInt32(price.flag_at12 ? 1U : 0U);
  writer.WriteUInt16(price.type);
  writer.WriteUInt32(price.duration);
  writer.WriteUInt32(price.show_price);
  writer.WriteUInt32(price.sell_price);
}

ShopPriceRecord ReadPrice(PacketReader& reader) {
  ShopPriceRecord price;
  price.stock_id = reader.ReadUInt32();
  price.item_id = reader.ReadUInt32();
  price.flag_at12 = reader.ReadUInt32() != 0;
  price.type = reader.ReadUInt16();
  price.duration = reader.ReadUInt32();
  price.show_price = reader.ReadUInt32();
  price.sell_price = reader.ReadUInt32();
  return price;
}
}  // namespace

TcpPacket ShopEntryResponse::Encode() const {
  // Preflight all nested counts and UTF-16 strings before allocating the
  // output buffer. These are local bounds; opaque values are preserved.
  std::size_t entries = 0;
  std::size_t bytes = 40;
  CountEntries(items.size(), entries);
  for (const auto& item : items) {
    CountBytes(kItemMinimumBytes, bytes);
    CountText(item.name, bytes);
    CountEntries(item.prices.size(), entries);
    CountBytes(kPriceBytes * item.prices.size(), bytes);
  }
  CountEntries(categories.size(), entries);
  for (const auto& category : categories) {
    CountBytes(kCategoryMinimumBytes, bytes);
    CountText(category.name, bytes);
    CountEntries(category.sub_categories.size(), entries);
    for (const auto& sub_category : category.sub_categories) {
      CountBytes(kCategoryMinimumBytes, bytes);
      CountText(sub_category.name, bytes);
    }
  }
  CountEntries(values_at56.size(), entries);
  CountBytes(4 * values_at56.size(), bytes);

  PacketWriter writer;
  writer.WriteUInt32(value_at0);
  writer.WriteUInt32(refresh_catalog ? 1U : 0U);
  writer.WriteUInt64(catalog_revision);
  writer.WriteUInt32(static_cast<std::uint32_t>(items.size()));
  for (const auto& item : items) {
    writer.WriteUInt32(item.identifier);
    writer.WriteUInt32(item.kind);
    writer.WriteUtf16(item.name);
    writer.WriteUInt32(item.value_at36);
    writer.WriteUInt32(item.main_category);
    writer.WriteUInt32(item.sub_category);
    writer.WriteUInt32(item.flag_at48 ? 1U : 0U);
    writer.WriteUInt64(item.wide_at56);
    writer.WriteUInt64(item.wide_at64);
    WritePrice(writer, item.price);
    writer.WriteUInt32(static_cast<std::uint32_t>(item.prices.size()));
    for (const auto& price : item.prices) {
      WritePrice(writer, price);
    }
  }
  writer.WriteUInt64(wide_at32);
  writer.WriteUInt32(static_cast<std::uint32_t>(categories.size()));
  for (const auto& category : categories) {
    writer.WriteUInt32(category.identifier);
    writer.WriteUtf16(category.name);
    writer.WriteUInt32(category.display_order);
    writer.WriteUInt32(
        static_cast<std::uint32_t>(category.sub_categories.size()));
    for (const auto& sub_category : category.sub_categories) {
      writer.WriteUInt32(sub_category.identifier);
      writer.WriteUtf16(sub_category.name);
      writer.WriteUInt32(sub_category.value_at32);
      writer.WriteUInt32(sub_category.value_at36);
    }
  }
  writer.WriteUInt32(static_cast<std::uint32_t>(values_at56.size()));
  for (const auto value : values_at56) {
    writer.WriteUInt32(value);
  }
  writer.WriteUInt32(selected_category);
  return {kMessageId, writer.TakeBytes()};
}

ShopEntryResponse ShopEntryResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Incorrect shop entry response message.");
  }
  if (packet.payload.size() > kMaxPayloadBytes) {
    throw std::invalid_argument("Shop response exceeds local byte limit.");
  }
  PacketReader reader(packet.payload);
  std::size_t entries = 0;
  ShopEntryResponse response;
  response.value_at0 = reader.ReadUInt32();
  response.refresh_catalog = reader.ReadUInt32() != 0;
  response.catalog_revision = reader.ReadUInt64();
  const auto item_count = ReadCount(reader, kItemMinimumBytes, entries);
  response.items.reserve(item_count);
  for (std::uint32_t index = 0; index < item_count; ++index) {
    ShopItemCardRecord item;
    item.identifier = reader.ReadUInt32();
    item.kind = reader.ReadUInt32();
    item.name = reader.ReadUtf16();
    item.value_at36 = reader.ReadUInt32();
    item.main_category = reader.ReadUInt32();
    item.sub_category = reader.ReadUInt32();
    item.flag_at48 = reader.ReadUInt32() != 0;
    item.wide_at56 = reader.ReadUInt64();
    item.wide_at64 = reader.ReadUInt64();
    item.price = ReadPrice(reader);
    const auto price_count = ReadCount(reader, kPriceBytes, entries);
    item.prices.reserve(price_count);
    for (std::uint32_t price_index = 0; price_index < price_count;
         ++price_index) {
      item.prices.push_back(ReadPrice(reader));
    }
    response.items.push_back(std::move(item));
  }
  response.wide_at32 = reader.ReadUInt64();
  const auto category_count = ReadCount(reader, kCategoryMinimumBytes, entries);
  response.categories.reserve(category_count);
  for (std::uint32_t index = 0; index < category_count; ++index) {
    ShopMainCategoryRecord category;
    category.identifier = reader.ReadUInt32();
    category.name = reader.ReadUtf16();
    category.display_order = reader.ReadUInt32();
    const auto sub_count = ReadCount(reader, kCategoryMinimumBytes, entries);
    category.sub_categories.reserve(sub_count);
    for (std::uint32_t sub_index = 0; sub_index < sub_count; ++sub_index) {
      ShopSubCategoryRecord sub_category;
      sub_category.identifier = reader.ReadUInt32();
      sub_category.name = reader.ReadUtf16();
      sub_category.value_at32 = reader.ReadUInt32();
      sub_category.value_at36 = reader.ReadUInt32();
      category.sub_categories.push_back(std::move(sub_category));
    }
    response.categories.push_back(std::move(category));
  }
  const auto value_count = ReadCount(reader, 4, entries);
  response.values_at56.reserve(value_count);
  for (std::uint32_t index = 0; index < value_count; ++index) {
    response.values_at56.push_back(reader.ReadUInt32());
  }
  response.selected_category = reader.ReadUInt32();
  reader.RequireEnd();
  return response;
}
}  // namespace bubble_fighter::server::protocol
