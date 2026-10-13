#include "protocol/characters/fighter_info.h"

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

void CheckCount(std::size_t count, std::size_t& remaining) {
  if (count > FighterInfo::kMaxCollectionEntries || count > remaining) {
    throw std::length_error("Fighter info exceeds supported collection limit.");
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
    throw std::runtime_error("Truncated fighter info collection.");
  }
  std::vector<Entry> entries;
  entries.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    entries.push_back(read_entry());
  }
  return entries;
}
}  // namespace

void FighterInfo::Write(PacketWriter& writer) const {
  if ((marker_at48 == kPresentMarker) != optional_at48.has_value() ||
      (marker_at328 == kPresentMarker) != optional_at328.has_value()) {
    throw std::invalid_argument(
        "Fighter info optional fields do not match their markers.");
  }
  std::size_t remaining = kMaxTotalCollectionEntries;
  writer.WriteUInt32(value_at4);
  writer.WriteUtf16(text_at8);
  WriteValues(writer, values_at32);
  writer.WriteUInt8(marker_at48);
  if (optional_at48) {
    writer.WriteUInt64(optional_at48->wide_value);
    WriteValues(writer, optional_at48->values);
    writer.WriteUInt32(optional_at48->boolean_value);
  }
  WriteValues(writer, values_at80);
  WriteEntries(writer, entries_at304, remaining,
               [&](const auto& entry) { WriteValues(writer, entry); });
  WriteValues(writer, values_at296);
  writer.WriteUInt32(value_at316);
  writer.WriteUInt8(byte_at320);
  writer.WriteUInt32(value_at324);
  writer.WriteUInt8(marker_at328);
  if (optional_at328) {
    writer.WriteUInt8(*optional_at328);
  }
  writer.WriteUInt32(value_at332);
  writer.WriteUInt8(byte_at336);
  writer.WriteUInt32(value_at340);
  WriteValues(writer, social.values);
  writer.WriteUInt32(social.boolean_value);
  writer.WriteUInt64(social.wide_value);
  WriteValues(writer, social.boolean_values);
  WriteValues(writer, values_at404);
  const auto write_rank = [&](const FighterMapRankEntry& entry) {
    writer.WriteUInt32(entry.key);
    WriteValues(writer, entry.values);
  };
  WriteEntries(writer, area32_ranks, remaining, write_rank);
  WriteValues(writer, values_at444);
  WriteEntries(writer, zombie_escape_ranks, remaining, write_rank);
  writer.WriteUtf16(text_at504);
  writer.WriteUInt32(value_at400);
  // Original wire order swaps object offsets +468 and +472.
  writer.WriteUInt32(values_at464[0]);
  writer.WriteUInt32(values_at464[2]);
  writer.WriteUInt32(values_at464[1]);
  for (std::size_t index = 3; index < values_at464.size(); ++index) {
    writer.WriteUInt32(values_at464[index]);
  }
  WriteEntries(writer, skill_slots, remaining,
               [&](const FighterSkillSlotGroup& group) {
                 writer.WriteUInt32(group.key);
                 WriteEntries(writer, group.entries, remaining,
                              [&](const FighterSkillSlotEntry& entry) {
                                writer.WriteUInt32(entry.key);
                                WriteValues(writer, entry.values);
                              });
               });
  WriteEntries(writer, abilities, remaining,
               [&](const FighterAbilityEntry& entry) {
                 writer.WriteUInt16(entry.key);
                 writer.WriteUInt32(entry.value);
               });
  WriteValues(writer, values_at552);
  WriteEntries(writer, groups_at564, remaining,
               [&](const FighterInfoGroup& group) {
                 writer.WriteUInt16(group.key);
                 WriteEntries(writer, group.entries, remaining,
                              [&](const FighterInfoGroupedEntry& entry) {
                                writer.WriteUInt16(entry.first_value);
                                writer.WriteUInt16(entry.second_value);
                                writer.WriteUInt32(entry.third_value);
                              });
               });
}

FighterInfo FighterInfo::Read(PacketReader& reader) {
  FighterInfo value;
  std::size_t remaining = kMaxTotalCollectionEntries;
  value.value_at4 = reader.ReadUInt32();
  value.text_at8 = reader.ReadUtf16();
  ReadValues(reader, value.values_at32);
  value.marker_at48 = reader.ReadUInt8();
  if (value.marker_at48 == kPresentMarker) {
    value.optional_at48.emplace();
    value.optional_at48->wide_value = reader.ReadUInt64();
    ReadValues(reader, value.optional_at48->values);
    value.optional_at48->boolean_value = reader.ReadUInt32();
  }
  ReadValues(reader, value.values_at80);
  value.entries_at304 =
      ReadEntries<std::array<std::uint32_t, 5>>(reader, remaining, 20, [&] {
        std::array<std::uint32_t, 5> entry{};
        ReadValues(reader, entry);
        return entry;
      });
  ReadValues(reader, value.values_at296);
  value.value_at316 = reader.ReadUInt32();
  value.byte_at320 = reader.ReadUInt8();
  value.value_at324 = reader.ReadUInt32();
  value.marker_at328 = reader.ReadUInt8();
  if (value.marker_at328 == kPresentMarker) {
    value.optional_at328 = reader.ReadUInt8();
  }
  value.value_at332 = reader.ReadUInt32();
  value.byte_at336 = reader.ReadUInt8();
  value.value_at340 = reader.ReadUInt32();
  ReadValues(reader, value.social.values);
  value.social.boolean_value = reader.ReadUInt32();
  value.social.wide_value = reader.ReadUInt64();
  ReadValues(reader, value.social.boolean_values);
  ReadValues(reader, value.values_at404);
  const auto read_rank = [&] {
    FighterMapRankEntry entry;
    entry.key = reader.ReadUInt32();
    ReadValues(reader, entry.values);
    return entry;
  };
  value.area32_ranks =
      ReadEntries<FighterMapRankEntry>(reader, remaining, 12, read_rank);
  ReadValues(reader, value.values_at444);
  value.zombie_escape_ranks =
      ReadEntries<FighterMapRankEntry>(reader, remaining, 12, read_rank);
  value.text_at504 = reader.ReadUtf16();
  value.value_at400 = reader.ReadUInt32();
  value.values_at464[0] = reader.ReadUInt32();
  value.values_at464[2] = reader.ReadUInt32();
  value.values_at464[1] = reader.ReadUInt32();
  for (std::size_t index = 3; index < value.values_at464.size(); ++index) {
    value.values_at464[index] = reader.ReadUInt32();
  }
  value.skill_slots =
      ReadEntries<FighterSkillSlotGroup>(reader, remaining, 8, [&] {
        FighterSkillSlotGroup group;
        group.key = reader.ReadUInt32();
        group.entries =
            ReadEntries<FighterSkillSlotEntry>(reader, remaining, 24, [&] {
              FighterSkillSlotEntry entry;
              entry.key = reader.ReadUInt32();
              ReadValues(reader, entry.values);
              return entry;
            });
        return group;
      });
  value.abilities = ReadEntries<FighterAbilityEntry>(reader, remaining, 6, [&] {
    FighterAbilityEntry entry;
    entry.key = reader.ReadUInt16();
    entry.value = reader.ReadUInt32();
    return entry;
  });
  ReadValues(reader, value.values_at552);
  value.groups_at564 = ReadEntries<FighterInfoGroup>(reader, remaining, 6, [&] {
    FighterInfoGroup group;
    group.key = reader.ReadUInt16();
    group.entries =
        ReadEntries<FighterInfoGroupedEntry>(reader, remaining, 8, [&] {
          FighterInfoGroupedEntry entry;
          entry.first_value = reader.ReadUInt16();
          entry.second_value = reader.ReadUInt16();
          entry.third_value = reader.ReadUInt32();
          return entry;
        });
    return group;
  });
  return value;
}
}  // namespace bubble_fighter::server::protocol
