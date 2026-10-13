#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_MEMBER_LEFT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_MEMBER_LEFT_NOTIFICATION_H_

#include <cstdint>

#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/scenes/goto_room_notification.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R16: S511, room dispatcher 0x009D9930 case 0x1FF ->
// 0x009DB1E0. Takes the previous occupant out of the slot (peer/UI
// cleanup through 0x00921670 and 0x0090E8E0), stores the sent member in
// its place and installs the team object. Departures send an empty member.
struct RoomMemberLeftNotification {
  static constexpr std::uint16_t kMessageId = 0x01FF;

  std::uint32_t slot = 0;
  SessionMember member;
  SessionTeam team;

  TcpPacket Encode() const;
  static RoomMemberLeftNotification Decode(const TcpPacket& packet);
  bool operator==(const RoomMemberLeftNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_MEMBER_LEFT_NOTIFICATION_H_
