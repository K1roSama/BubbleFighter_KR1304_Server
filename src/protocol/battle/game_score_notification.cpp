#include "protocol/battle/game_score_notification.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
TcpPacket GameScoreNotification::Encode() const {
  if (entries_at19188.size() > kMaxEntries) {
    throw std::length_error("Game score map exceeds local limit.");
  }
  std::size_t size = scores.EncodedSize() + 17 + entries_at19188.size() * 12 +
                     (extension_tag == 0x3A ? 12 : 0);
  if (size > kMaxPayloadBytes) {
    throw std::length_error("Game score notification too large.");
  }
  for (const auto& text : texts_at19200) {
    if ((flags_at19272 & 4) == 0) {
      if (!text.empty()) {
        throw std::invalid_argument("Game score texts require flag bit 2.");
      }
    } else {
      if (text.size() > PacketWriter::kMaxStringCodeUnits ||
          size > kMaxPayloadBytes - 4 ||
          text.size() > (kMaxPayloadBytes - size - 4) / 2) {
        throw std::length_error("Game score extension text too large.");
      }
      size += 4 + text.size() * 2;
    }
  }
  if (extension_tag != 0x3A &&
      values_at19276 != std::array<std::uint32_t, 3>{}) {
    throw std::invalid_argument(
        "Game score extension values require tag 0x3A.");
  }
  PacketWriter writer;
  scores.Write(writer);
  writer.WriteUInt32(value_at19180);
  writer.WriteUInt32(elapsed_milliseconds);
  writer.WriteUInt32(static_cast<std::uint32_t>(entries_at19188.size()));
  for (const auto& [key, values] : entries_at19188) {
    writer.WriteUInt32(key);
    writer.WriteUInt32(values[0]);
    writer.WriteUInt32(values[1]);
  }
  writer.WriteUInt8(byte_at19196);
  writer.WriteUInt8(byte_at19197);
  writer.WriteUInt16(flags_at19272);
  if ((flags_at19272 & 4) != 0) {
    for (const auto& text : texts_at19200) {
      writer.WriteUtf16(text);
    }
  }
  writer.WriteUInt8(extension_tag);
  if (extension_tag == 0x3A) {
    for (const auto value : values_at19276) {
      writer.WriteUInt32(value);
    }
  }
  return {kMessageId, writer.TakeBytes()};
}

GameScoreNotification GameScoreNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game score message.");
  }
  if (packet.payload.size() > kMaxPayloadBytes) {
    throw std::length_error("Game score notification too large.");
  }
  PacketReader reader(packet.payload);
  GameScoreNotification result;
  result.scores = GameScoreRecord::Read(reader);
  result.value_at19180 = reader.ReadUInt32();
  result.elapsed_milliseconds = reader.ReadUInt32();
  const auto count = reader.ReadUInt32();
  if (count > kMaxEntries || count > reader.RemainingBytes() / 12) {
    throw std::length_error("Game score map invalid or truncated.");
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto key = reader.ReadUInt32();
    const std::array<std::uint32_t, 2> values{reader.ReadUInt32(),
                                              reader.ReadUInt32()};
    // Native map keeps the first duplicate; this implementation rejects
    // ambiguity.
    if (!result.entries_at19188.emplace(key, values).second) {
      throw std::invalid_argument("Duplicate game score map key.");
    }
  }
  result.byte_at19196 = reader.ReadUInt8();
  result.byte_at19197 = reader.ReadUInt8();
  result.flags_at19272 = reader.ReadUInt16();
  if ((result.flags_at19272 & 4) != 0) {
    for (auto& text : result.texts_at19200) {
      text = reader.ReadUtf16();
    }
  }
  result.extension_tag = reader.ReadUInt8();
  if (result.extension_tag == 0x3A) {
    for (auto& value : result.values_at19276) {
      value = reader.ReadUInt32();
    }
  }
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
