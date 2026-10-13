#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_SESSION_DESCRIPTOR_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_SESSION_DESCRIPTOR_H_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace bubble_fighter::server::protocol {
class PacketReader;
class PacketWriter;

struct SessionTextEntry {
  std::uint32_t value = 0;
  std::u16string text;
  bool operator==(const SessionTextEntry&) const = default;
};

struct SessionValueGroup {
  std::uint16_t key = 0;
  std::vector<std::uint32_t> values;
  bool operator==(const SessionValueGroup&) const = default;
};

// E045/E047: At<number> names CLIENT OBJECT offsets, not wire offsets.
// Boolean fields retain their uint32 wire representation; the client
// normalizes nonzero values to true. Defaults below are codec defaults,
// not a server-approved room/plaza configuration.
struct SessionConfiguration {
  std::uint32_t value_at0 =
      0;  // Session flags; full bit semantics remain incomplete.
  std::uint16_t short_at4 = 0;  // Mode; native reader sign-extends to int32.
  std::uint32_t boolean_at8 = 0;
  std::array<std::uint32_t, 6> values_at20{};
  std::array<std::u16string, 4> texts_at44;
  std::uint32_t value_at140 = 0;
  std::array<std::u16string, 6> texts_at144;
  std::array<std::uint32_t, 2> values_at288{};
  std::uint32_t boolean_at296 = 0;
  std::vector<std::array<std::uint32_t, 2>> pairs_at297;
  std::uint32_t boolean_at309 = 0;
  std::vector<SessionTextEntry> entries_at310;
  std::array<std::uint32_t, 5> booleans_at322{};
  std::vector<SessionValueGroup> groups_at328;
  std::array<std::uint32_t, 2> booleans_at340{};
  std::uint8_t byte_at344 = 0;
  std::array<std::uint32_t, 2> values_at348{};
  std::array<std::u16string, 2> texts_at356;
  std::uint32_t value_at404 = 0;
  bool operator==(const SessionConfiguration&) const = default;
};

// Shared descriptor prefix of S117. This is not the full notification:
// its members, maps, team data and trailing fields must follow separately.
struct SessionDescriptor {
  static constexpr std::uint32_t kMaxCollectionEntries = 1024;
  static constexpr std::uint32_t kMaxTotalCollectionEntries = 4096;

  std::uint16_t short_at0 = 0;
  std::uint32_t value_at4 = 0;
  std::u16string text_at8;
  std::array<std::uint16_t, 3> shorts_at32{};
  std::uint32_t boolean_at38 = 0;
  std::u16string text_at40;
  // Handler 0x009AC490: 1 -> stage 2, 3 -> stage 3, 6 -> stage 15.
  // Type 2 usually selects stage 4, but mode 40 (and other special modes)
  // skips that switch while status != 4 and waits for later loading.
  std::uint32_t session_type = 0;
  SessionConfiguration configuration;
  std::uint32_t value_at68 = 0;
  std::array<std::uint32_t, 2> flags_at72{};
  std::uint64_t wide_at80 = 0;
  std::u16string text_at88;
  std::uint32_t boolean_at112 = 0;
  std::array<std::uint32_t, 3> values_at116{};
  std::uint16_t short_at128 = 0;

  void Write(PacketWriter& writer) const;
  static SessionDescriptor Read(PacketReader& reader);
  bool operator==(const SessionDescriptor&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_SESSION_DESCRIPTOR_H_
