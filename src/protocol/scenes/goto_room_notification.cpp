#include "protocol/scenes/goto_room_notification.h"

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

void Charge(std::size_t count, std::size_t& budget) {
  if (count > GotoRoomNotification::kMaxCollectionEntries || count > budget) {
    throw std::length_error("Session snapshot exceeds local collection limit.");
  }
  budget -= count;
}

template <typename Entry, typename WriteEntry>
void WriteEntries(PacketWriter& writer, const std::vector<Entry>& entries,
                  std::size_t& budget, WriteEntry write_entry) {
  Charge(entries.size(), budget);
  writer.WriteUInt32(static_cast<std::uint32_t>(entries.size()));
  for (const auto& entry : entries) {
    write_entry(entry);
  }
}

template <typename Entry, typename ReadEntry>
std::vector<Entry> ReadEntries(PacketReader& reader, std::size_t& budget,
                               std::size_t minimum_bytes,
                               ReadEntry read_entry) {
  const auto count = reader.ReadUInt32();
  Charge(count, budget);
  if (count > reader.RemainingBytes() / minimum_bytes) {
    throw std::runtime_error("Truncated session snapshot collection.");
  }
  std::vector<Entry> entries;
  entries.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    entries.push_back(read_entry());
  }
  return entries;
}

void WriteVector(PacketWriter& writer, const std::vector<std::uint32_t>& values,
                 std::size_t& budget) {
  WriteEntries(writer, values, budget,
               [&](const auto kValue) { writer.WriteUInt32(kValue); });
}

std::vector<std::uint32_t> ReadVector(PacketReader& reader,
                                      std::size_t& budget) {
  return ReadEntries<std::uint32_t>(reader, budget, 4,
                                    [&] { return reader.ReadUInt32(); });
}

template <typename Value, typename WriteValue>
void WriteOptional(PacketWriter& writer, std::uint8_t marker,
                   const std::optional<Value>& value, WriteValue write_value) {
  if ((marker == GotoRoomNotification::kPresentMarker) != value.has_value()) {
    throw std::invalid_argument(
        "Session snapshot optional value does not match its marker.");
  }
  writer.WriteUInt8(marker);
  if (value) {
    write_value(*value);
  }
}

void WriteMember(PacketWriter& writer, const SessionMember& value,
                 std::size_t& budget) {
  WriteValues(writer, value.booleans_at0);
  writer.WriteUInt8(value.byte_at3);
  writer.WriteUInt8(value.signed_byte_at4);
  writer.WriteUInt32(value.value_at8);
  writer.WriteUtf16(value.text_at12);
  for (const auto item : value.signed_shorts_at36) {
    writer.WriteUInt16(item);
  }
  WriteValues(writer, value.booleans_at96);
  WriteValues(writer, value.values_at44);
  WriteVector(writer, value.values_at52, budget);
  WriteValues(writer, value.values_at64);
  writer.WriteUInt32(value.value_at88);
  writer.WriteUInt32(value.boolean_at94);
  writer.WriteUInt32(value.value_at100);
  writer.WriteUInt32(value.boolean_at104);
  writer.WriteUInt32(value.value_at108);
  writer.WriteUInt32(value.boolean_at112);
  writer.WriteUtf16(value.text_at116);
  writer.WriteUInt32(value.value_at140);
  writer.WriteUtf16(value.text_at144);
  WriteValues(writer, value.booleans_at168);
  WriteValues(writer, value.values_at172);
  writer.WriteUInt8(value.byte_at184);
  writer.WriteUInt32(value.boolean_at185);
  WriteValues(writer, value.values_at188);
  WriteOptional(
      writer, value.marker_at204, value.optional_at204,
      [&](const auto& fields) {
        WriteValues(writer, fields.values_at204);
        WriteOptional(
            writer, fields.marker_at212, fields.optional_at212,
            [&](const auto& entries) { WriteValues(writer, entries); });
      });
  writer.WriteUInt32(value.boolean_at232);
  WriteValues(writer, value.social.values);
  writer.WriteUInt32(value.social.boolean_value);
  writer.WriteUInt64(value.social.wide_value);
  WriteValues(writer, value.social.boolean_values);
}

SessionMember ReadMember(PacketReader& reader, std::size_t& budget) {
  SessionMember value;
  ReadValues(reader, value.booleans_at0);
  value.byte_at3 = reader.ReadUInt8();
  value.signed_byte_at4 = reader.ReadUInt8();
  value.value_at8 = reader.ReadUInt32();
  value.text_at12 = reader.ReadUtf16();
  for (auto& item : value.signed_shorts_at36) {
    item = reader.ReadUInt16();
  }
  ReadValues(reader, value.booleans_at96);
  ReadValues(reader, value.values_at44);
  value.values_at52 = ReadVector(reader, budget);
  ReadValues(reader, value.values_at64);
  value.value_at88 = reader.ReadUInt32();
  value.boolean_at94 = reader.ReadUInt32();
  value.value_at100 = reader.ReadUInt32();
  value.boolean_at104 = reader.ReadUInt32();
  value.value_at108 = reader.ReadUInt32();
  value.boolean_at112 = reader.ReadUInt32();
  value.text_at116 = reader.ReadUtf16();
  value.value_at140 = reader.ReadUInt32();
  value.text_at144 = reader.ReadUtf16();
  ReadValues(reader, value.booleans_at168);
  ReadValues(reader, value.values_at172);
  value.byte_at184 = reader.ReadUInt8();
  value.boolean_at185 = reader.ReadUInt32();
  ReadValues(reader, value.values_at188);
  value.marker_at204 = reader.ReadUInt8();
  if (value.marker_at204 == GotoRoomNotification::kPresentMarker) {
    auto& fields = value.optional_at204.emplace();
    ReadValues(reader, fields.values_at204);
    fields.marker_at212 = reader.ReadUInt8();
    if (fields.marker_at212 == GotoRoomNotification::kPresentMarker) {
      ReadValues(reader, fields.optional_at212.emplace());
    }
  }
  value.boolean_at232 = reader.ReadUInt32();
  ReadValues(reader, value.social.values);
  value.social.boolean_value = reader.ReadUInt32();
  value.social.wide_value = reader.ReadUInt64();
  ReadValues(reader, value.social.boolean_values);
  return value;
}

void WriteFields(PacketWriter& writer, const SessionOptionalFields& value,
                 std::size_t& budget) {
  writer.WriteUInt32(value.value_at0);
  writer.WriteUInt64(value.wide_at8);
  WriteEntries(writer, value.texts_at16, budget,
               [&](const auto& text) { writer.WriteUtf16(text); });
  WriteOptional(writer, value.marker_at28, value.optional_at28,
                [&](const auto& group) {
                  writer.WriteUInt32(group.value);
                  WriteVector(writer, group.values, budget);
                });
  WriteValues(writer, value.values_at48);
}

SessionOptionalFields ReadFields(PacketReader& reader, std::size_t& budget) {
  SessionOptionalFields value;
  value.value_at0 = reader.ReadUInt32();
  value.wide_at8 = reader.ReadUInt64();
  value.texts_at16 = ReadEntries<std::u16string>(
      reader, budget, 4, [&] { return reader.ReadUtf16(); });
  value.marker_at28 = reader.ReadUInt8();
  if (value.marker_at28 == GotoRoomNotification::kPresentMarker) {
    auto& group = value.optional_at28.emplace();
    group.value = reader.ReadUInt32();
    group.values = ReadVector(reader, budget);
  }
  ReadValues(reader, value.values_at48);
  return value;
}
}  // namespace

void WriteSessionMember(PacketWriter& writer, const SessionMember& value,
                        std::size_t& budget) {
  WriteMember(writer, value, budget);
}
SessionMember ReadSessionMember(PacketReader& reader, std::size_t& budget) {
  return ReadMember(reader, budget);
}
void WriteSessionRecords(PacketWriter& writer,
                         const std::vector<PartySupplementaryRecord>& records,
                         std::size_t& budget) {
  WriteEntries(writer, records, budget,
               [&](const auto& record) { record.Write(writer, budget); });
}
std::vector<PartySupplementaryRecord> ReadSessionRecords(PacketReader& reader,
                                                         std::size_t& budget) {
  return ReadEntries<PartySupplementaryRecord>(reader, budget, 28, [&] {
    return PartySupplementaryRecord::Read(reader, budget);
  });
}
void WriteSessionTeam(PacketWriter& writer, const SessionTeam& team,
                      std::size_t& budget) {
  WriteEntries(writer, team.groups, budget, [&](const SessionTeamGroup& group) {
    writer.WriteUInt8(group.signed_key);
    WriteVector(writer, group.values, budget);
  });
  for (const auto& values : team.values) {
    WriteVector(writer, values, budget);
  }
}
SessionTeam ReadSessionTeam(PacketReader& reader, std::size_t& budget) {
  SessionTeam team;
  team.groups = ReadEntries<SessionTeamGroup>(reader, budget, 5, [&] {
    SessionTeamGroup group;
    group.signed_key = reader.ReadUInt8();
    group.values = ReadVector(reader, budget);
    return group;
  });
  for (auto& values : team.values) {
    values = ReadVector(reader, budget);
  }
  return team;
}

TcpPacket GotoRoomNotification::Encode() const {
  PacketWriter writer;
  std::size_t budget = kMaxTotalCollectionEntries;
  descriptor.Write(writer);
  writer.WriteUInt32(value_at544);
  writer.WriteUInt32(value_at548);
  WriteEntries(writer, members, budget, [&](const auto& member) {
    WriteMember(writer, member, budget);
  });
  WriteSessionRecords(writer, records_at564, budget);
  WriteSessionRecords(writer, records_at588, budget);
  WriteSessionTeam(writer, team, budget);
  WriteOptional(writer, marker_at648, optional_at648, [&](const auto& fields) {
    WriteFields(writer, fields, budget);
  });
  writer.WriteUInt32(value_at728);
  writer.WriteUInt32(boolean_at732);
  return {kMessageId, writer.TakeBytes()};
}

GotoRoomNotification GotoRoomNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument(
        "Unexpected goto-room notification message id.");
  }
  PacketReader reader(packet.payload);
  std::size_t budget = kMaxTotalCollectionEntries;
  GotoRoomNotification message;
  message.descriptor = SessionDescriptor::Read(reader);
  message.value_at544 = reader.ReadUInt32();
  message.value_at548 = reader.ReadUInt32();
  message.members = ReadEntries<SessionMember>(
      reader, budget, 188, [&] { return ReadMember(reader, budget); });
  message.records_at564 = ReadSessionRecords(reader, budget);
  message.records_at588 = ReadSessionRecords(reader, budget);
  message.team = ReadSessionTeam(reader, budget);
  message.marker_at648 = reader.ReadUInt8();
  if (message.marker_at648 == kPresentMarker) {
    message.optional_at648 = ReadFields(reader, budget);
  }
  message.value_at728 = reader.ReadUInt32();
  message.boolean_at732 = reader.ReadUInt32();
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
