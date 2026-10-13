#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_RECORD_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_RECORD_H_
#include <array>
#include <map>
#include <string>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
// E073: native ScoreData player objects have stride 512. Offsets below
// describe the client object, not padding or a native server structure.
struct GameScorePlayerRecord {
  std::uint32_t value_at500 = 0;
  std::uint32_t value_at0 = 0;
  std::u16string text_at4;
  std::uint8_t byte_at52 = 0;
  static constexpr std::array<std::size_t, 29> kBeforeTextOffsets{
      56,  60,  64,  68,  80,  72,  84,  88,  92,  96,  104, 100, 108, 112, 120,
      124, 128, 176, 132, 136, 140, 364, 368, 372, 376, 380, 384, 388, 436};
  // Offset 388 is a native bool, zero-extended to uint32 on the wire.
  // Keep the raw wire value; reading it does not authorize any reward.
  std::array<std::uint32_t, 29> values_before_texts{};
  std::vector<std::u16string> texts_at464;
  std::u16string text_at440;
  static constexpr std::array<std::size_t, 7> kAfterTextOffsets{
      236, 476, 480, 392, 144, 148, 408};
  std::array<std::uint32_t, 7> values_after_texts{};
  std::uint16_t short_at412 = 0;
  static constexpr std::array<std::size_t, 5> kAfterShortOffsets{416, 420, 160,
                                                                 164, 168};
  std::array<std::uint32_t, 5> values_after_short{};
  std::array<std::uint8_t, 2> bytes_at172{};
  std::uint16_t short_at184 = 0;
  std::uint16_t short_at76 = 0;
  std::uint16_t short_at186 = 0;
  std::uint32_t value_at508 = 0;
  std::uint16_t short_at188 = 0;
  std::uint8_t byte_at432 = 0;
  static constexpr std::array<std::size_t, 10> kTailOffsets{
      192, 196, 200, 204, 208, 212, 216, 220, 424, 428};
  std::array<std::uint32_t, 10> tail_values{};
  // Two separate passes after all player records, indexed by the same mask.
  std::uint32_t value_in_first_tail = 0;   // ScoreData+18892+slot*4
  std::uint32_t value_in_second_tail = 0;  // ScoreData+19036+slot*4
  bool operator==(const GameScorePlayerRecord&) const = default;
};

struct GameScoreRecord {
  static constexpr std::size_t kSlotCount = 36;
  static constexpr std::size_t kMaxPayloadBytes =
      65536;  // Local allocation policy.
  static constexpr std::size_t kMaxTotalTexts =
      1024;  // Across all player string lists.
  // Native count is a signed byte, followed by this raw byte and uint32.
  // Values are not named winner/team/reward without consumer evidence.
  std::uint8_t byte_at452 = 0;
  std::uint32_t value_at456 = 0;
  std::vector<std::array<std::uint32_t, 3>> triples;
  std::map<std::uint8_t, GameScorePlayerRecord> players;

  std::size_t EncodedSize() const;
  void Write(PacketWriter& writer) const;
  static GameScoreRecord Read(PacketReader& reader);
  bool operator==(const GameScoreRecord&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_SCORE_RECORD_H_
