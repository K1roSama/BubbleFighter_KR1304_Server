#include "protocol/scenes/session_descriptor.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
namespace {
template <std::size_t Size>
void WriteValues(PacketWriter& writer,
                 const std::array<std::uint32_t, Size>& values) {
  for (const auto value : values) {
    writer.WriteUInt32(value);
  }
}

template <std::size_t Size>
void ReadValues(PacketReader& reader, std::array<std::uint32_t, Size>& values) {
  for (auto& value : values) {
    value = reader.ReadUInt32();
  }
}

template <std::size_t Size>
void WriteTexts(PacketWriter& writer,
                const std::array<std::u16string, Size>& texts) {
  for (const auto& text : texts) {
    writer.WriteUtf16(text);
  }
}

template <std::size_t Size>
void ReadTexts(PacketReader& reader, std::array<std::u16string, Size>& texts) {
  for (auto& text : texts) {
    text = reader.ReadUtf16();
  }
}

void CheckCount(std::size_t count, std::size_t& remaining) {
  if (count > SessionDescriptor::kMaxCollectionEntries || count > remaining) {
    throw std::length_error(
        "Session descriptor exceeds supported collection limit.");
  }
  remaining -= count;
}

template <typename Entry, typename WriteEntry>
void WriteEntries(PacketWriter& writer, const std::vector<Entry>& entries,
                  std::size_t& remaining, WriteEntry write_entry) {
  CheckCount(entries.size(), remaining);
  writer.WriteUInt32(static_cast<std::uint32_t>(entries.size()));
  for (const auto& entry : entries) {
    write_entry(entry);
  }
}

template <typename Entry, typename ReadEntry>
std::vector<Entry> ReadEntries(PacketReader& reader, std::size_t& remaining,
                               std::size_t minimum_bytes,
                               ReadEntry read_entry) {
  const auto count = reader.ReadUInt32();
  CheckCount(count, remaining);
  if (count > reader.RemainingBytes() / minimum_bytes) {
    throw std::runtime_error("Truncated session descriptor collection.");
  }
  std::vector<Entry> entries;
  entries.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    entries.push_back(read_entry());
  }
  return entries;
}

void WriteConfiguration(PacketWriter& writer,
                        const SessionConfiguration& value) {
  std::size_t remaining = SessionDescriptor::kMaxTotalCollectionEntries;
  writer.WriteUInt32(value.value_at0);
  writer.WriteUInt16(value.short_at4);
  writer.WriteUInt32(value.boolean_at8);
  WriteValues(writer, value.values_at20);
  WriteTexts(writer, value.texts_at44);
  writer.WriteUInt32(value.value_at140);
  WriteTexts(writer, value.texts_at144);
  WriteValues(writer, value.values_at288);
  writer.WriteUInt32(value.boolean_at296);
  WriteEntries(writer, value.pairs_at297, remaining,
               [&](const auto& entry) { WriteValues(writer, entry); });
  writer.WriteUInt32(value.boolean_at309);
  WriteEntries(writer, value.entries_at310, remaining,
               [&](const SessionTextEntry& entry) {
                 writer.WriteUInt32(entry.value);
                 writer.WriteUtf16(entry.text);
               });
  WriteValues(writer, value.booleans_at322);
  WriteEntries(writer, value.groups_at328, remaining,
               [&](const SessionValueGroup& group) {
                 writer.WriteUInt16(group.key);
                 WriteEntries(
                     writer, group.values, remaining,
                     [&](const auto kEntry) { writer.WriteUInt32(kEntry); });
               });
  WriteValues(writer, value.booleans_at340);
  writer.WriteUInt8(value.byte_at344);
  WriteValues(writer, value.values_at348);
  WriteTexts(writer, value.texts_at356);
  writer.WriteUInt32(value.value_at404);
}

SessionConfiguration ReadConfiguration(PacketReader& reader) {
  SessionConfiguration value;
  std::size_t remaining = SessionDescriptor::kMaxTotalCollectionEntries;
  value.value_at0 = reader.ReadUInt32();
  value.short_at4 = reader.ReadUInt16();
  value.boolean_at8 = reader.ReadUInt32();
  ReadValues(reader, value.values_at20);
  ReadTexts(reader, value.texts_at44);
  value.value_at140 = reader.ReadUInt32();
  ReadTexts(reader, value.texts_at144);
  ReadValues(reader, value.values_at288);
  value.boolean_at296 = reader.ReadUInt32();
  value.pairs_at297 =
      ReadEntries<std::array<std::uint32_t, 2>>(reader, remaining, 8, [&] {
        return std::array<std::uint32_t, 2>{reader.ReadUInt32(),
                                            reader.ReadUInt32()};
      });
  value.boolean_at309 = reader.ReadUInt32();
  value.entries_at310 =
      ReadEntries<SessionTextEntry>(reader, remaining, 8, [&] {
        return SessionTextEntry{reader.ReadUInt32(), reader.ReadUtf16()};
      });
  ReadValues(reader, value.booleans_at322);
  value.groups_at328 =
      ReadEntries<SessionValueGroup>(reader, remaining, 6, [&] {
        SessionValueGroup group;
        group.key = reader.ReadUInt16();
        group.values = ReadEntries<std::uint32_t>(
            reader, remaining, 4, [&] { return reader.ReadUInt32(); });
        return group;
      });
  ReadValues(reader, value.booleans_at340);
  value.byte_at344 = reader.ReadUInt8();
  ReadValues(reader, value.values_at348);
  ReadTexts(reader, value.texts_at356);
  value.value_at404 = reader.ReadUInt32();
  return value;
}
}  // namespace

void SessionDescriptor::Write(PacketWriter& writer) const {
  writer.WriteUInt16(short_at0);
  writer.WriteUInt32(value_at4);
  writer.WriteUtf16(text_at8);
  for (const auto value : shorts_at32) {
    writer.WriteUInt16(value);
  }
  writer.WriteUInt32(boolean_at38);
  writer.WriteUtf16(text_at40);
  writer.WriteUInt32(session_type);
  WriteConfiguration(writer, configuration);
  writer.WriteUInt32(value_at68);
  WriteValues(writer, flags_at72);
  writer.WriteUInt64(wide_at80);
  writer.WriteUtf16(text_at88);
  writer.WriteUInt32(boolean_at112);
  WriteValues(writer, values_at116);
  writer.WriteUInt16(short_at128);
}

SessionDescriptor SessionDescriptor::Read(PacketReader& reader) {
  SessionDescriptor value;
  value.short_at0 = reader.ReadUInt16();
  value.value_at4 = reader.ReadUInt32();
  value.text_at8 = reader.ReadUtf16();
  for (auto& item : value.shorts_at32) {
    item = reader.ReadUInt16();
  }
  value.boolean_at38 = reader.ReadUInt32();
  value.text_at40 = reader.ReadUtf16();
  value.session_type = reader.ReadUInt32();
  value.configuration = ReadConfiguration(reader);
  value.value_at68 = reader.ReadUInt32();
  ReadValues(reader, value.flags_at72);
  value.wide_at80 = reader.ReadUInt64();
  value.text_at88 = reader.ReadUtf16();
  value.boolean_at112 = reader.ReadUInt32();
  ReadValues(reader, value.values_at116);
  value.short_at128 = reader.ReadUInt16();
  return value;
}
}  // namespace bubble_fighter::server::protocol
