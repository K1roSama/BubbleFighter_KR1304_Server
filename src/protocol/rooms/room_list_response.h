#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_LIST_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_LIST_RESPONSE_H_

#include <cstdint>
#include <vector>

#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/scenes/session_descriptor.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R15: S101, LobbyStage 0x00853E90 case 0x65 -> reader
// 0x0084F550 -> list update 0x00856CD0 -> renderer 0x0063CF90.
struct RoomListResponse {
  static constexpr std::uint16_t kMessageId = 0x0065;
  // Local resource limit; descriptors are read by 0x010D5210.
  static constexpr std::uint32_t kMaxRooms = 1024;

  // Shown page; the renderer enables "previous" for a nonzero value.
  std::uint32_t page = 0;
  // Stored as a bool at reader +4; enables the "next" button.
  std::uint32_t has_next_page = 0;
  // Read into reader +8 and passed to 0x0063CF90, which never uses it.
  std::uint32_t value_at8 = 0;
  // Each entry is the same descriptor a room's S117 carries.
  std::vector<SessionDescriptor> rooms;

  TcpPacket Encode() const;
  static RoomListResponse Decode(const TcpPacket& packet);
  bool operator==(const RoomListResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_LIST_RESPONSE_H_
