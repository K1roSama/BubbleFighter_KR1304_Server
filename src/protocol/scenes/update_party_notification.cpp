#include "protocol/scenes/update_party_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
namespace {
void Charge(std::size_t count, std::size_t& budget) {
  if (count > UpdatePartyNotification::kMaxCollectionEntries ||
      count > budget) {
    throw std::invalid_argument(
        "Party collection exceeds local resource limit.");
  }
  budget -= count;
}
std::uint32_t ReadCount(PacketReader& reader, std::size_t minimum_bytes,
                        std::size_t& budget) {
  const auto count = reader.ReadUInt32();
  Charge(count, budget);
  if (count > reader.RemainingBytes() / minimum_bytes) {
    throw std::invalid_argument("Truncated party collection.");
  }
  return count;
}
void WriteCount(PacketWriter& writer, std::size_t count, std::size_t& budget) {
  Charge(count, budget);
  writer.WriteUInt32(static_cast<std::uint32_t>(count));
}
}  // namespace

void PartySupplementaryRecord::Write(PacketWriter& writer,
                                     std::size_t& budget) const {
  writer.WriteUInt32(key);
  writer.WriteUInt32(value);
  for (const auto& group : groups) {
    WriteCount(writer, group.size(), budget);
    for (const auto group_value : group) {
      writer.WriteUInt32(group_value);
    }
  }
  WriteCount(writer, bytes.size(), budget);
  for (const auto byte : bytes) {
    writer.WriteUInt8(byte);
  }
  WriteCount(writer, details.size(), budget);
  for (const auto& detail : details) {
    for (const auto detail_value : detail.values) {
      writer.WriteUInt32(detail_value);
    }
    WriteCount(writer, detail.entries.size(), budget);
    for (const auto entry_value : detail.entries) {
      writer.WriteUInt32(entry_value);
    }
    writer.WriteUInt32(detail.final_value);
    writer.WriteUtf16(detail.text);
  }
}

PartySupplementaryRecord PartySupplementaryRecord::Read(PacketReader& reader,
                                                        std::size_t& budget) {
  PartySupplementaryRecord record;
  record.key = reader.ReadUInt32();
  record.value = reader.ReadUInt32();
  for (auto& group : record.groups) {
    const auto size = ReadCount(reader, 4, budget);
    group.reserve(size);
    for (std::uint32_t j = 0; j < size; ++j) {
      group.push_back(reader.ReadUInt32());
    }
  }
  const auto bytes = ReadCount(reader, 1, budget);
  record.bytes.reserve(bytes);
  for (std::uint32_t j = 0; j < bytes; ++j) {
    record.bytes.push_back(reader.ReadUInt8());
  }
  const auto details = ReadCount(reader, 20, budget);
  record.details.reserve(details);
  for (std::uint32_t j = 0; j < details; ++j) {
    auto& detail = record.details.emplace_back();
    for (auto& value : detail.values) {
      value = reader.ReadUInt32();
    }
    const auto entries = ReadCount(reader, 4, budget);
    detail.entries.reserve(entries);
    for (std::uint32_t k = 0; k < entries; ++k) {
      detail.entries.push_back(reader.ReadUInt32());
    }
    detail.final_value = reader.ReadUInt32();
    detail.text = reader.ReadUtf16();
  }
  return record;
}

TcpPacket UpdatePartyNotification::Encode() const {
  PacketWriter writer;
  std::size_t budget = kMaxTotalEntries;
  writer.WriteUInt32(state);
  WriteCount(writer, members.size(), budget);
  for (const auto& member : members) {
    writer.WriteUInt32(member.identity_value);
    writer.WriteUtf16(member.character_name);
    for (const auto value : member.values_at32) {
      writer.WriteUInt32(value);
    }
  }
  WriteCount(writer, supplementary_records.size(), budget);
  for (const auto& record : supplementary_records) {
    record.Write(writer, budget);
  }
  return {kMessageId, writer.TakeBytes()};
}

UpdatePartyNotification UpdatePartyNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected party message id.");
  }
  PacketReader reader(packet.payload);
  std::size_t budget = kMaxTotalEntries;
  UpdatePartyNotification result;
  result.state = reader.ReadUInt32();
  auto count = ReadCount(reader, 24, budget);
  result.members.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    auto& member = result.members.emplace_back();
    member.identity_value = reader.ReadUInt32();
    member.character_name = reader.ReadUtf16();
    for (auto& value : member.values_at32) {
      value = reader.ReadUInt32();
    }
  }
  count = ReadCount(reader, 28, budget);
  result.supplementary_records.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    auto& record = result.supplementary_records.emplace_back();
    record = PartySupplementaryRecord::Read(reader, budget);
  }
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
