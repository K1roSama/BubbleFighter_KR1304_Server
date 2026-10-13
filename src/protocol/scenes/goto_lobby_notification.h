#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_GOTO_LOBBY_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_GOTO_LOBBY_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E043: uint32; nonzero displays the party-moved-to-lobby notice.
// Both branches request stage 3 when the main connection is ready.
struct GotoLobbyNotification {
  static constexpr std::uint16_t kMessageId = 0x0074;
  std::uint32_t party_move_notice = 0;
  TcpPacket Encode() const;
  static GotoLobbyNotification Decode(const TcpPacket& packet);
  bool operator==(const GotoLobbyNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_GOTO_LOBBY_NOTIFICATION_H_
