#include "protocol/battle/game_score_value_update.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
void GameScoreValueUpdate::WriteList(
    PacketWriter& writer, std::span<const GameScoreValueUpdate> updates) {
  if (updates.size() > kMaxTotalEntries) {
    throw std::length_error("Score update list exceeds local limit.");
  }
  writer.WriteUInt32(static_cast<std::uint32_t>(updates.size()));
  for (const auto& update : updates) {
    writer.WriteUInt8(update.index);
    writer.WriteUInt16(update.value);
  }
}

std::vector<GameScoreValueUpdate> GameScoreValueUpdate::ReadList(
    PacketReader& reader, std::size_t& budget) {
  const auto count = reader.ReadUInt32();
  if (count > kMaxTotalEntries || count > budget ||
      count > reader.RemainingBytes() / 3) {
    throw std::length_error(
        "Score update count exceeds budget or remaining bytes.");
  }
  budget -= count;
  std::vector<GameScoreValueUpdate> updates;
  updates.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    updates.push_back({reader.ReadUInt8(), reader.ReadUInt16()});
  }
  return updates;
}
}  // namespace bubble_fighter::server::protocol
