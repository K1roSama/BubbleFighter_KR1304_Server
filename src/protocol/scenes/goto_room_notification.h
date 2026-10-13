#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_GOTO_ROOM_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_GOTO_ROOM_NOTIFICATION_H_

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "protocol/characters/fighter_info.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/scenes/session_descriptor.h"
#include "protocol/scenes/update_party_notification.h"

namespace bubble_fighter::server::protocol {
class PacketReader;
class PacketWriter;
struct SessionMemberOptionalFields {
  std::array<std::uint32_t, 2> values_at204{};
  std::uint8_t marker_at212 = 0xBA;
  std::optional<std::array<std::uint32_t, 2>> optional_at212;
  bool operator==(const SessionMemberOptionalFields&) const = default;
};

// E047: reader 0x01143710. Offsets refer to the x86 client object.
// Signed byte/short fields retain their wire bits; uint32 booleans retain
// noncanonical values. Zero initialization is not a usable player profile.
struct SessionMember {
  std::array<std::uint32_t, 3> booleans_at0{};
  std::uint8_t byte_at3 = 0;
  std::uint8_t signed_byte_at4 = 0;
  // E050: occupied flag is BooleansAt0[0]; +8/+12 are player ID/name.
  std::uint32_t value_at8 = 0;
  std::u16string text_at12;
  // E050: second short becomes the color palette index at client +40.
  std::array<std::uint16_t, 2> signed_shorts_at36{};
  std::array<std::uint32_t, 3> booleans_at96{};
  // E050: these and ValuesAt52/ValuesAt64/ValueAt88 feed UDP peer setup.
  std::array<std::uint32_t, 2> values_at44{};
  std::vector<std::uint32_t> values_at52;
  std::array<std::uint32_t, 2> values_at64{};
  std::uint32_t value_at88 = 0;
  std::uint32_t boolean_at94 = 0;
  std::uint32_t value_at100 = 0;
  std::uint32_t boolean_at104 = 0;
  std::uint32_t value_at108 = 0;
  std::uint32_t boolean_at112 = 0;
  std::u16string text_at116;
  std::uint32_t value_at140 = 0;
  std::u16string text_at144;
  std::array<std::uint32_t, 2> booleans_at168{};
  // E051: first value is the ordinary LevelSymbolIcon level; remaining
  // values select/affect alternative icon modes and stay explicit.
  std::array<std::uint32_t, 3> values_at172{};
  std::uint8_t byte_at184 = 0;
  std::uint32_t boolean_at185 = 0;
  std::array<std::uint32_t, 4> values_at188{};
  std::uint8_t marker_at204 = 0xBA;
  std::optional<SessionMemberOptionalFields> optional_at204;
  std::uint32_t boolean_at232 = 0;
  SocialSystemInfo social;
  bool operator==(const SessionMember&) const = default;
};

struct SessionTeamGroup {
  std::uint8_t signed_key = 0;
  std::vector<std::uint32_t> values;
  bool operator==(const SessionTeamGroup&) const = default;
};

struct SessionTeam {
  std::vector<SessionTeamGroup> groups;
  std::array<std::vector<std::uint32_t>, 2> values;
  bool operator==(const SessionTeam&) const = default;
};

// Shared original record codecs used by S117, S503 and S511.
void WriteSessionMember(PacketWriter& writer, const SessionMember& value,
                        std::size_t& budget);
SessionMember ReadSessionMember(PacketReader& reader, std::size_t& budget);
void WriteSessionRecords(PacketWriter& writer,
                         const std::vector<PartySupplementaryRecord>& records,
                         std::size_t& budget);
std::vector<PartySupplementaryRecord> ReadSessionRecords(PacketReader& reader,
                                                         std::size_t& budget);
void WriteSessionTeam(PacketWriter& writer, const SessionTeam& team,
                      std::size_t& budget);
SessionTeam ReadSessionTeam(PacketReader& reader, std::size_t& budget);

struct SessionOptionalValueGroup {
  std::uint32_t value = 0;
  std::vector<std::uint32_t> values;
  bool operator==(const SessionOptionalValueGroup&) const = default;
};

struct SessionOptionalFields {
  std::uint32_t value_at0 = 0;
  std::uint64_t wide_at8 = 0;
  std::vector<std::u16string> texts_at16;
  std::uint8_t marker_at28 = 0xBA;
  std::optional<SessionOptionalValueGroup> optional_at28;
  std::array<std::uint32_t, 5> values_at48{};
  bool operator==(const SessionOptionalFields&) const = default;
};

// E045/E047: gspGotoRoom, also used for menu/plaza stage initialization.
// Used by authenticated local menu/plaza initialization.
struct GotoRoomNotification {
  static constexpr std::uint16_t kMessageId = 0x0075;
  static constexpr std::uint8_t kPresentMarker = 0x3A;
  static constexpr std::uint8_t kAbsentMarker = 0xBA;
  static constexpr std::uint32_t kMaxCollectionEntries = 1024;
  // Descriptor and remaining payload each have an independent 4096 budget.
  static constexpr std::uint32_t kMaxTotalCollectionEntries = 4096;

  SessionDescriptor descriptor;
  std::uint32_t value_at544 = 0;
  std::uint32_t value_at548 = 0;
  std::vector<SessionMember> members;
  // E050: first map keyed by slot index, second map by player ID.
  std::vector<PartySupplementaryRecord> records_at564;
  std::vector<PartySupplementaryRecord> records_at588;
  SessionTeam team;
  std::uint8_t marker_at648 = kAbsentMarker;
  std::optional<SessionOptionalFields> optional_at648;
  // E050: local slot index, passed to context constructor 0x009D9250.
  // Do not substitute the old version's second word after the descriptor.
  std::uint32_t value_at728 = 0;
  std::uint32_t boolean_at732 = 0;

  TcpPacket Encode() const;
  static GotoRoomNotification Decode(const TcpPacket& packet);
  bool operator==(const GotoRoomNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_GOTO_ROOM_NOTIFICATION_H_
