#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_GAME_PATCH_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_GAME_PATCH_RESPONSE_H_

#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct GamePatchResponse {
  static constexpr std::uint16_t kMessageId = 0x0002;
  // E011: zero continues to stage 10; nonzero selects the error branch.
  std::uint32_t result = 0;
  std::u16string message;

  TcpPacket Encode() const;
  static GamePatchResponse Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_GAME_PATCH_RESPONSE_H_
