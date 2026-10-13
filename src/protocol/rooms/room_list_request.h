#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_LIST_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_LIST_REQUEST_H_

#include <cstdint>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R15: C401, written only by 0x00858E70 (callers: room list
// widget 0x0063E5D0 and LobbyStage 0x008530C0/0x00853930).
struct RoomListRequest {
  static constexpr std::uint16_t kMessageId = 0x0191;
  // 0x00858E70 writes this constant after the page index.
  static constexpr std::uint16_t kClientPageSize = 10;

  // Zero-based: 0x0063CF90 enables the previous-page button for page > 0.
  std::uint16_t page = 0;
  std::uint16_t page_size = kClientPageSize;

  TcpPacket Encode() const;
  static RoomListRequest Decode(const TcpPacket& packet);
  bool operator==(const RoomListRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_ROOM_LIST_REQUEST_H_
