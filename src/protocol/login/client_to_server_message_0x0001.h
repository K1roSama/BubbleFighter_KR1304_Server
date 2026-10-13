#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CLIENT_TO_SERVER_MESSAGE_0X0001_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CLIENT_TO_SERVER_MESSAGE_0X0001_H_

#include <array>
#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct ClientToServerMessage0x0001 {
  static constexpr std::uint16_t kMessageId = 0x0001;
  std::uint32_t initial_value = 0;
  std::array<std::u16string, 22> string_fields;
  std::uint32_t intermediate_value = 0;

  TcpPacket Encode() const;
  static ClientToServerMessage0x0001 Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CLIENT_TO_SERVER_MESSAGE_0X0001_H_
