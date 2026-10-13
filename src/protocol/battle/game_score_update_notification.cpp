#include "protocol/battle/game_score_update_notification.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
void GameScoreUpdateNotification::Validate() const {
  if (group_index > 36 || (first_slot >= 36 && first_slot < 128) ||
      (second_slot >= 36 && second_slot < 128)) {
    throw std::invalid_argument(
        "Score update group or player exceeds native capacity.");
  }
  std::size_t budget = GameScoreValueUpdate::kMaxTotalEntries;
  for (const auto* updates :
       {&first_updates, &second_updates, &group_updates}) {
    if (updates->size() > budget) {
      throw std::length_error("Score update exceeds total entry budget.");
    }
    budget -= updates->size();
  }
  for (const auto& update : group_updates) {
    // This list sign-extends its byte key and indexes a native 37-entry array.
    if (update.index > 36) {
      throw std::invalid_argument("Invalid score update group index.");
    }
  }
}

TcpPacket GameScoreUpdateNotification::Encode() const {
  Validate();
  PacketWriter writer;
  writer.WriteUInt8(group_index);
  writer.WriteUInt16(group_value);
  writer.WriteUInt8(first_slot);
  writer.WriteUInt8(second_slot);
  GameScoreValueUpdate::WriteList(writer, first_updates);
  GameScoreValueUpdate::WriteList(writer, second_updates);
  GameScoreValueUpdate::WriteList(writer, group_updates);
  return {kMessageId, writer.TakeBytes()};
}

GameScoreUpdateNotification GameScoreUpdateNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid score update message.");
  }
  if (packet.payload.size() > kMaxPayloadBytes) {
    throw std::length_error("Score update payload exceeds local limit.");
  }
  PacketReader reader(packet.payload);
  GameScoreUpdateNotification result;
  result.group_index = reader.ReadUInt8();
  result.group_value = reader.ReadUInt16();
  result.first_slot = reader.ReadUInt8();
  result.second_slot = reader.ReadUInt8();
  std::size_t budget = GameScoreValueUpdate::kMaxTotalEntries;
  result.first_updates = GameScoreValueUpdate::ReadList(reader, budget);
  result.second_updates = GameScoreValueUpdate::ReadList(reader, budget);
  result.group_updates = GameScoreValueUpdate::ReadList(reader, budget);
  reader.RequireEnd();
  result.Validate();
  return result;
}
}  // namespace bubble_fighter::server::protocol
