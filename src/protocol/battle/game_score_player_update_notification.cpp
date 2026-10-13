#include "protocol/battle/game_score_player_update_notification.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
TcpPacket GameScorePlayerUpdateNotification::Encode() const {
  if (players.size() > kMaxPlayers) {
    throw std::length_error("Score update map exceeds native slot count.");
  }
  std::size_t budget = GameScoreValueUpdate::kMaxTotalEntries;
  for (const auto& [slot, updates] : players) {
    if (slot >= kMaxPlayers) {
      throw std::invalid_argument("Invalid score update player slot.");
    }
    if (updates.size() > budget) {
      throw std::length_error("Score update exceeds total entry budget.");
    }
    budget -= updates.size();
  }
  PacketWriter writer;
  writer.WriteUInt32(static_cast<std::uint32_t>(players.size()));
  for (const auto& [slot, updates] : players) {
    writer.WriteUInt32(slot);
    GameScoreValueUpdate::WriteList(writer, updates);
  }
  return {kMessageId, writer.TakeBytes()};
}

GameScorePlayerUpdateNotification GameScorePlayerUpdateNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid player score update message.");
  }
  if (packet.payload.size() > kMaxPayloadBytes) {
    throw std::length_error("Player score update payload exceeds local limit.");
  }
  PacketReader reader(packet.payload);
  const auto count = reader.ReadUInt32();
  if (count > kMaxPlayers || count > reader.RemainingBytes() / 8) {
    throw std::length_error("Score update player count invalid or truncated.");
  }
  GameScorePlayerUpdateNotification result;
  std::size_t budget = GameScoreValueUpdate::kMaxTotalEntries;
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto slot = reader.ReadUInt32();
    if (slot >= kMaxPlayers || result.players.contains(slot)) {
      throw std::invalid_argument(
          "Invalid or repeated score update player slot.");
    }
    result.players.emplace(slot,
                           GameScoreValueUpdate::ReadList(reader, budget));
  }
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
