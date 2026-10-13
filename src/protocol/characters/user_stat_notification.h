#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_USER_STAT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_USER_STAT_NOTIFICATION_H_

#include "protocol/characters/fighter_info.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E026: gspUserStat, original outer reader at 0x009D8030.
struct UserStatNotification {
  static constexpr std::uint16_t kMessageId = 0x0069;

  // E051: [0] color, [1] experience, [4] Lucci, [5] Cash.
  // RoomSnapshot: [2]/[3] experience range base/next, supplied by the
  // configured local policy. [6]/[7] remain explicit/unknown.
  std::array<std::uint32_t, 8> values{};
  std::u16string text;
  std::uint16_t short_value = 0;
  FighterInfo fighter;

  TcpPacket Encode() const;
  static UserStatNotification Decode(const TcpPacket& packet);
  bool operator==(const UserStatNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_USER_STAT_NOTIFICATION_H_
