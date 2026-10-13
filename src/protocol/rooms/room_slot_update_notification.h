#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_SLOT_UPDATE_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_SLOT_UPDATE_NOTIFICATION_H_

#include <cstdint>
#include <vector>

#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/scenes/goto_room_notification.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R16: S503, room dispatcher 0x009D9930 case 0x1F7 ->
// 0x009DA720. Replaces one slot's member (a previously empty slot is a
// join), merges both equipment maps entry by entry (0x0081D060 by slot,
// 0x0081D010 by player) and installs the team object (0x009EF840).
struct RoomSlotUpdateNotification {
  static constexpr std::uint16_t kMessageId = 0x01F7;

  std::uint32_t slot = 0;
  SessionMember member;
  std::vector<PartySupplementaryRecord> records_at564;
  std::vector<PartySupplementaryRecord> records_at588;
  SessionTeam team;

  TcpPacket Encode() const;
  static RoomSlotUpdateNotification Decode(const TcpPacket& packet);
  bool operator==(const RoomSlotUpdateNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_SLOT_UPDATE_NOTIFICATION_H_
