#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_FIGHTER_INFO_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_FIGHTER_INFO_H_

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace bubble_fighter::server::protocol {
class PacketReader;
class PacketWriter;

struct FighterInfoOptionalFields {
  std::uint64_t wide_value = 0;
  std::array<std::uint32_t, 2> values{};
  std::uint32_t boolean_value = 0;
  bool operator==(const FighterInfoOptionalFields&) const = default;
};

struct SocialSystemInfo {
  std::array<std::uint32_t, 7> values{};
  std::uint32_t boolean_value = 0;
  std::uint64_t wide_value = 0;
  std::array<std::uint32_t, 2> boolean_values{};
  bool operator==(const SocialSystemInfo&) const = default;
};

struct FighterMapRankEntry {
  std::uint32_t key = 0;
  std::array<std::uint32_t, 2> values{};
  bool operator==(const FighterMapRankEntry&) const = default;
};

struct FighterSkillSlotEntry {
  std::uint32_t key = 0;
  std::array<std::uint32_t, 5> values{};
  bool operator==(const FighterSkillSlotEntry&) const = default;
};

struct FighterSkillSlotGroup {
  std::uint32_t key = 0;
  std::vector<FighterSkillSlotEntry> entries;
  bool operator==(const FighterSkillSlotGroup&) const = default;
};

struct FighterAbilityEntry {
  std::uint16_t key = 0;
  std::uint32_t value = 0;
  bool operator==(const FighterAbilityEntry&) const = default;
};

struct FighterInfoGroupedEntry {
  std::uint16_t first_value = 0;
  std::uint16_t second_value = 0;
  std::uint32_t third_value = 0;
  bool operator==(const FighterInfoGroupedEntry&) const = default;
};

struct FighterInfoGroup {
  std::uint16_t key = 0;
  std::vector<FighterInfoGroupedEntry> entries;
  bool operator==(const FighterInfoGroup&) const = default;
};

// E026: complete BFCLIENT02 wire layout; most business meanings remain unknown.
// At<number> refers to a CLIENT OBJECT offset, never a network offset.
// Collections preserve wire order and keys, including noncanonical map input.
struct FighterInfo {
  static constexpr std::uint8_t kPresentMarker = 0x3A;
  static constexpr std::uint8_t kAbsentMarker = 0xBA;
  // Local resource limits, not established limits of the original server.
  static constexpr std::uint32_t kMaxCollectionEntries = 1024;
  static constexpr std::uint32_t kMaxTotalCollectionEntries = 4096;

  std::uint32_t value_at4 = 0;
  std::u16string text_at8;
  // E051: index 1 (+36) is level. Other indices remain offset-named.
  std::array<std::uint32_t, 3> values_at32{};
  std::uint8_t marker_at48 = kAbsentMarker;
  std::optional<FighterInfoOptionalFields> optional_at48;
  std::array<std::uint32_t, 54> values_at80{};
  std::vector<std::array<std::uint32_t, 5>> entries_at304;
  std::array<std::uint32_t, 2> values_at296{};
  std::uint32_t value_at316 = 0;
  std::uint8_t byte_at320 = 0;
  std::uint32_t value_at324 = 0;
  std::uint8_t marker_at328 = kAbsentMarker;
  std::optional<std::uint8_t> optional_at328;
  std::uint32_t value_at332 = 0;
  std::uint8_t byte_at336 = 0;
  std::uint32_t value_at340 = 0;
  SocialSystemInfo social;
  std::array<std::uint32_t, 8> values_at404{};
  std::vector<FighterMapRankEntry> area32_ranks;
  std::array<std::uint32_t, 3> values_at444{};
  std::vector<FighterMapRankEntry> zombie_escape_ranks;
  std::u16string text_at504;
  std::uint32_t value_at400 = 0;
  std::array<std::uint32_t, 10> values_at464{};
  std::vector<FighterSkillSlotGroup> skill_slots;
  std::vector<FighterAbilityEntry> abilities;
  std::array<std::uint32_t, 3> values_at552{};
  std::vector<FighterInfoGroup> groups_at564;

  void Write(PacketWriter& writer) const;
  static FighterInfo Read(PacketReader& reader);
  bool operator==(const FighterInfo&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_FIGHTER_INFO_H_
