#include "protocol/battle/game_score_record.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
namespace {
template <std::size_t N>
void WriteValues(PacketWriter& writer,
                 const std::array<std::uint32_t, N>& values) {
  for (const auto value : values) {
    writer.WriteUInt32(value);
  }
}
template <std::size_t N>
void ReadValues(PacketReader& reader, std::array<std::uint32_t, N>& values) {
  for (auto& value : values) {
    value = reader.ReadUInt32();
  }
}
void AddTextSize(const std::u16string& text, std::size_t& size) {
  if (text.size() > PacketWriter::kMaxStringCodeUnits ||
      size > GameScoreRecord::kMaxPayloadBytes - 4 ||
      text.size() > (GameScoreRecord::kMaxPayloadBytes - size - 4) / 2) {
    throw std::length_error("Game score exceeds local string/payload limit.");
  }
  size += 4 + text.size() * 2;
}
}  // namespace

std::size_t GameScoreRecord::EncodedSize() const {
  if (triples.size() > kSlotCount || players.size() > kSlotCount) {
    throw std::length_error("Game score exceeds native slot capacity.");
  }
  std::size_t size = 14 + triples.size() * 12;
  std::size_t text_count = 0;
  for (const auto& [slot, p] : players) {
    if (slot >= kSlotCount) {
      throw std::invalid_argument("Invalid game score slot.");
    }
    if (p.texts_at464.size() > kMaxTotalTexts - text_count) {
      throw std::length_error(
          "Game score text collection exceeds local limit.");
    }
    text_count += p.texts_at464.size();
    // Scalar fields, list count and both final tail passes; strings counted
    // below.
    constexpr std::size_t fixed_bytes = 242;
    if (size > kMaxPayloadBytes - fixed_bytes) {
      throw std::length_error("Game score payload too large.");
    }
    size += fixed_bytes;
    AddTextSize(p.text_at4, size);
    for (const auto& text : p.texts_at464) {
      AddTextSize(text, size);
    }
    AddTextSize(p.text_at440, size);
  }
  return size;
}

void GameScoreRecord::Write(PacketWriter& writer) const {
  EncodedSize();  // Validate before appending anything.
  writer.WriteUInt8(static_cast<std::uint8_t>(triples.size()));
  writer.WriteUInt8(byte_at452);
  writer.WriteUInt32(value_at456);
  for (const auto& triple : triples) {
    WriteValues(writer, triple);
  }
  std::uint64_t mask = 0;
  for (const auto& [slot, player] : players) {
    (void)player;
    mask |= std::uint64_t{1} << slot;
  }
  writer.WriteUInt64(mask);
  for (const auto& [slot, p] : players) {
    (void)slot;
    writer.WriteUInt32(p.value_at500);
    writer.WriteUInt32(p.value_at0);
    writer.WriteUtf16(p.text_at4);
    writer.WriteUInt8(p.byte_at52);
    WriteValues(writer, p.values_before_texts);
    writer.WriteUInt32(static_cast<std::uint32_t>(p.texts_at464.size()));
    for (const auto& text : p.texts_at464) {
      writer.WriteUtf16(text);
    }
    writer.WriteUtf16(p.text_at440);
    WriteValues(writer, p.values_after_texts);
    writer.WriteUInt16(p.short_at412);
    WriteValues(writer, p.values_after_short);
    for (const auto value : p.bytes_at172) {
      writer.WriteUInt8(value);
    }
    writer.WriteUInt16(p.short_at184);
    writer.WriteUInt16(p.short_at76);
    writer.WriteUInt16(p.short_at186);
    writer.WriteUInt32(p.value_at508);
    writer.WriteUInt16(p.short_at188);
    writer.WriteUInt8(p.byte_at432);
    WriteValues(writer, p.tail_values);
  }
  for (const auto& [slot, p] : players) {
    (void)slot;
    writer.WriteUInt32(p.value_in_first_tail);
  }
  for (const auto& [slot, p] : players) {
    (void)slot;
    writer.WriteUInt32(p.value_in_second_tail);
  }
}

GameScoreRecord GameScoreRecord::Read(PacketReader& reader) {
  if (reader.RemainingBytes() > kMaxPayloadBytes) {
    throw std::length_error("Game score input exceeds local limit.");
  }
  GameScoreRecord result;
  const auto count = reader.ReadUInt8();
  if (count > kSlotCount) {
    throw std::length_error("Invalid native game score count.");
  }
  result.byte_at452 = reader.ReadUInt8();
  result.value_at456 = reader.ReadUInt32();
  for (std::uint8_t i = 0; i < count; ++i) {
    std::array<std::uint32_t, 3> triple{};
    ReadValues(reader, triple);
    result.triples.push_back(triple);
  }
  const auto mask = reader.ReadUInt64();
  if ((mask >> kSlotCount) != 0) {
    throw std::invalid_argument("Game score mask exceeds native slots.");
  }
  std::size_t text_count = 0;
  for (std::uint8_t slot = 0; slot < kSlotCount; ++slot) {
    if ((mask & (std::uint64_t{1} << slot)) == 0) {
      continue;
    }
    GameScorePlayerRecord p;
    p.value_at500 = reader.ReadUInt32();
    p.value_at0 = reader.ReadUInt32();
    p.text_at4 = reader.ReadUtf16();
    p.byte_at52 = reader.ReadUInt8();
    ReadValues(reader, p.values_before_texts);
    const auto texts = reader.ReadUInt32();
    if (texts > kMaxTotalTexts - text_count ||
        texts > reader.RemainingBytes() / 4) {
      throw std::length_error(
          "Game score text collection invalid or truncated.");
    }
    text_count += texts;
    for (std::uint32_t i = 0; i < texts; ++i) {
      p.texts_at464.push_back(reader.ReadUtf16());
    }
    p.text_at440 = reader.ReadUtf16();
    ReadValues(reader, p.values_after_texts);
    p.short_at412 = reader.ReadUInt16();
    ReadValues(reader, p.values_after_short);
    for (auto& value : p.bytes_at172) {
      value = reader.ReadUInt8();
    }
    p.short_at184 = reader.ReadUInt16();
    p.short_at76 = reader.ReadUInt16();
    p.short_at186 = reader.ReadUInt16();
    p.value_at508 = reader.ReadUInt32();
    p.short_at188 = reader.ReadUInt16();
    p.byte_at432 = reader.ReadUInt8();
    ReadValues(reader, p.tail_values);
    result.players.emplace(slot, std::move(p));
  }
  for (auto& [slot, p] : result.players) {
    (void)slot;
    p.value_in_first_tail = reader.ReadUInt32();
  }
  for (auto& [slot, p] : result.players) {
    (void)slot;
    p.value_in_second_tail = reader.ReadUInt32();
  }
  result.EncodedSize();
  return result;
}
}  // namespace bubble_fighter::server::protocol
