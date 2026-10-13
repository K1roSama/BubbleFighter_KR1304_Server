#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_JOIN_ROOM_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_JOIN_ROOM_RESPONSE_H_

#include <cstdint>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// RoomCreation R17: S103, LobbyStage 0x00853E90 case 0x67. Zero shows
// nothing; other codes select text in 0x00857990 (codes 11 and 20+ have no
// entry). Only the codes this server sends are named.
enum class JoinRoomResult : std::uint32_t {
  kJoined = 0,
  // "이미 게임이 시작된 방입니다."
  kGameStarted = 1,
  // "방 인원이 꽉 찼어요."
  kRoomFull = 3,
  // "방 비밀 번호가 틀렸습니다."
  kWrongPassword = 4,
  // "조건에 맞는 방이 없습니다."
  kNoMatchingRoom = 5,
};

struct JoinRoomResponse {
  static constexpr std::uint16_t kMessageId = 0x0067;

  std::uint32_t result = 0;
  // Only the level-limit text (code 19, "%d") formats a value.
  std::uint32_t detail_value = 0;

  TcpPacket Encode() const;
  static JoinRoomResponse Decode(const TcpPacket& packet);
  bool operator==(const JoinRoomResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_ROOMS_JOIN_ROOM_RESPONSE_H_
