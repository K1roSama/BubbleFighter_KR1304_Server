#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_CHAT_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_CHAT_REQUEST_H_
#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02 0x4E9070: channel, text, optional character expression.
struct ChatRequest {
  static constexpr std::uint16_t kMessageId = 502;
  std::uint32_t channel = 0;
  std::u16string text;
  std::u16string expression;
  TcpPacket Encode() const;
  static ChatRequest Decode(const TcpPacket& packet);
  bool operator==(const ChatRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_CHAT_REQUEST_H_
