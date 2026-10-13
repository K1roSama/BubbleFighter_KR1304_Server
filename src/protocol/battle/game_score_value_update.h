#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_VALUE_UPDATE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_VALUE_UPDATE_H_
#include <span>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
// E074: each list element is byte index + uint16 value, with no padding.
// A player's index selects a field; a group list's index selects a group.
struct GameScoreValueUpdate {
  static constexpr std::size_t kMaxTotalEntries =
      1024;  // Local packet-wide budget.
  std::uint8_t index = 0;
  std::uint16_t value = 0;
  static void WriteList(PacketWriter& writer,
                        std::span<const GameScoreValueUpdate> updates);
  static std::vector<GameScoreValueUpdate> ReadList(PacketReader& reader,
                                                    std::size_t& budget);
  bool operator==(const GameScoreValueUpdate&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_VALUE_UPDATE_H_
