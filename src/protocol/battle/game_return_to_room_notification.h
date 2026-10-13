#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RETURN_TO_ROOM_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RETURN_TO_ROOM_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 0x009E33B4 consumes no payload; 0x009EA690 clears the game
// and selects RoomStage=4 for the supported ordinary mode/flags branch.
struct GameReturnToRoomNotification {
  static constexpr std::uint16_t kMessageId = 620;
  TcpPacket Encode() const;
  static GameReturnToRoomNotification Decode(const TcpPacket& packet);
  bool operator==(const GameReturnToRoomNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_RETURN_TO_ROOM_NOTIFICATION_H_
