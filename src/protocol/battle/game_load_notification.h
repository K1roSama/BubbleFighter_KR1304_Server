#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_LOAD_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_LOAD_NOTIFICATION_H_

#include <array>
#include <map>
#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E068/E069: 0x009D9930 -> 0x010D1D70 -> 0x009E5E70 -> game stage 5.
// GameStage then runs internal loading steps; stage 5 is not load completion.
// At<number> denotes a CLIENT OBJECT offset, never wire padding.
// Unknown values are preserved; zero initialization is not a valid start
// policy.
struct GameLoadNotification {
  static constexpr std::uint16_t kMessageId = 510;
  static constexpr std::size_t kMaxPayloadBytes = 65536;
  static constexpr std::size_t kMaxCollectionEntries = 1024;
  static constexpr std::size_t kMaxTotalCollectionEntries = 1024;
  std::array<std::u16string, 3> texts_at0;
  std::uint32_t value_at72 = 0;
  std::uint32_t value_at76 = 0;
  std::uint8_t signed_byte_at80 = 0;  // Original reader sign-extends to int32.
  std::uint8_t flags_at84 = 0;
  std::uint32_t value_at88 = 0;
  std::uint8_t byte_at92 = 0;
  // Original readers sign-extend the byte keys to int32 before tree insertion.
  std::map<std::int8_t, std::array<std::uint32_t, 2>> entries_at96;
  std::map<std::uint32_t, std::array<std::uint32_t, 2>> entries_at104;
  std::map<std::int8_t, std::array<std::uint32_t, 2>> entries_at112;

  TcpPacket Encode() const;
  static GameLoadNotification Decode(const TcpPacket& packet);
  bool operator==(const GameLoadNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_LOAD_NOTIFICATION_H_
