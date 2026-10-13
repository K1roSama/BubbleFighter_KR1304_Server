#include "protocol/battle/game_load_notification.h"

#include <bit>
#include <stdexcept>
#include <type_traits>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
namespace {
void Charge(std::size_t count, std::size_t& budget) {
  if (count > GameLoadNotification::kMaxCollectionEntries || count > budget) {
    throw std::length_error("Game load collection exceeds local limit.");
  }
  budget -= count;
}

template <typename Key>
void WriteEntries(PacketWriter& writer,
                  const std::map<Key, std::array<std::uint32_t, 2>>& entries) {
  writer.WriteUInt32(static_cast<std::uint32_t>(entries.size()));
  for (const auto& [key, values] : entries) {
    if constexpr (std::is_same_v<Key, std::int8_t>) {
      writer.WriteUInt8(std::bit_cast<std::uint8_t>(key));
    } else {
      writer.WriteUInt32(key);
    }
    writer.WriteUInt32(values[0]);
    writer.WriteUInt32(values[1]);
  }
}

template <typename Key>
std::map<Key, std::array<std::uint32_t, 2>> ReadEntries(PacketReader& reader,
                                                        std::size_t& budget) {
  const auto count = reader.ReadUInt32();
  Charge(count, budget);
  constexpr std::size_t entry_size = std::is_same_v<Key, std::int8_t> ? 9 : 12;
  if (count > reader.RemainingBytes() / entry_size) {
    throw std::runtime_error("Truncated game load collection.");
  }
  std::map<Key, std::array<std::uint32_t, 2>> entries;
  for (std::uint32_t i = 0; i < count; ++i) {
    Key key;
    if constexpr (std::is_same_v<Key, std::int8_t>) {
      key = std::bit_cast<std::int8_t>(reader.ReadUInt8());
    } else {
      key = reader.ReadUInt32();
    }
    const std::array<std::uint32_t, 2> values{reader.ReadUInt32(),
                                              reader.ReadUInt32()};
    // Local strictness: original trees retain the first duplicate.
    if (!entries.emplace(key, values).second) {
      throw std::invalid_argument("Duplicate game load collection key.");
    }
  }
  return entries;
}
}  // namespace

TcpPacket GameLoadNotification::Encode() const {
  std::size_t budget = kMaxTotalCollectionEntries;
  Charge(entries_at96.size(), budget);
  Charge(entries_at104.size(), budget);
  Charge(entries_at112.size(), budget);
  std::size_t size = 39 + entries_at96.size() * 9 + entries_at104.size() * 12 +
                     entries_at112.size() * 9;
  for (const auto& text : texts_at0) {
    if (text.size() > PacketWriter::kMaxStringCodeUnits ||
        text.size() > (kMaxPayloadBytes - size) / 2) {
      throw std::length_error(
          "Game load exceeds local string or payload limit.");
    }
    size += text.size() * 2;
  }
  PacketWriter writer;
  for (const auto& text : texts_at0) {
    writer.WriteUtf16(text);
  }
  writer.WriteUInt32(value_at72);
  writer.WriteUInt32(value_at76);
  writer.WriteUInt8(signed_byte_at80);
  writer.WriteUInt8(flags_at84);
  writer.WriteUInt32(value_at88);
  writer.WriteUInt8(byte_at92);
  WriteEntries(writer, entries_at96);
  WriteEntries(writer, entries_at104);
  WriteEntries(writer, entries_at112);
  return {kMessageId, writer.TakeBytes()};
}

GameLoadNotification GameLoadNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game load message.");
  }
  if (packet.payload.size() > kMaxPayloadBytes) {
    throw std::length_error("Game load exceeds local payload limit.");
  }
  PacketReader reader(packet.payload);
  GameLoadNotification result;
  for (auto& text : result.texts_at0) {
    text = reader.ReadUtf16();
  }
  result.value_at72 = reader.ReadUInt32();
  result.value_at76 = reader.ReadUInt32();
  result.signed_byte_at80 = reader.ReadUInt8();
  result.flags_at84 = reader.ReadUInt8();
  result.value_at88 = reader.ReadUInt32();
  result.byte_at92 = reader.ReadUInt8();
  std::size_t budget = kMaxTotalCollectionEntries;
  result.entries_at96 = ReadEntries<std::int8_t>(reader, budget);
  result.entries_at104 = ReadEntries<std::uint32_t>(reader, budget);
  result.entries_at112 = ReadEntries<std::int8_t>(reader, budget);
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
