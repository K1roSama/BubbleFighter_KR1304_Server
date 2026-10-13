#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_UPDATE_PARTY_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_UPDATE_PARTY_NOTIFICATION_H_
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
class PacketReader;
class PacketWriter;
struct PartyMember {
  std::uint32_t identity_value = 0;
  std::u16string character_name;
  // E041: client offsets, not wire offsets; full semantics unknown.
  std::array<std::uint32_t, 4> values_at32{};
  bool operator==(const PartyMember&) const = default;
};

struct PartySupplementaryDetail {
  std::array<std::uint32_t, 2> values{};
  std::vector<std::uint32_t> entries;
  std::uint32_t final_value = 0;
  std::u16string text;
  bool operator==(const PartySupplementaryDetail&) const = default;
};

struct PartySupplementaryRecord {
  std::uint32_t key = 0;
  std::uint32_t value = 0;
  std::array<std::vector<std::uint32_t>, 3> groups;
  std::vector<std::uint8_t> bytes;
  std::vector<PartySupplementaryDetail> details;
  // Shared with S117 (E047); includes Key. The caller owns a cumulative
  // collection budget so nested records cannot reset allocation limits.
  void Write(PacketWriter& writer, std::size_t& budget) const;
  static PartySupplementaryRecord Read(PacketReader& reader,
                                       std::size_t& budget);
  bool operator==(const PartySupplementaryRecord&) const = default;
};

// E041: gspUpdateParty, including the trailing supplementary map.
// Preserve wire order. These local resource limits are not original rules.
struct UpdatePartyNotification {
  static constexpr std::uint16_t kMessageId = 0x00C9;
  static constexpr std::uint32_t kMaxCollectionEntries = 1024;
  static constexpr std::uint32_t kMaxTotalEntries = 4096;
  std::uint32_t state = 0;
  std::vector<PartyMember> members;
  std::vector<PartySupplementaryRecord> supplementary_records;
  TcpPacket Encode() const;
  static UpdatePartyNotification Decode(const TcpPacket& packet);
  bool operator==(const UpdatePartyNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_UPDATE_PARTY_NOTIFICATION_H_
