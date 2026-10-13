#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CHECK_CREATE_USER_NAME_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CHECK_CREATE_USER_NAME_RESPONSE_H_

#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E028: gspRepCheckCreateUserName, read at BFCLIENT02 0x009A4E90.
struct CheckCreateUserNameResponse {
  static constexpr std::uint16_t kMessageId = 0x000D;

  std::uint32_t result = 0;
  // Present even on failure; on acceptance this is used by the create request.
  std::u16string character_name;

  TcpPacket Encode() const;
  static CheckCreateUserNameResponse Decode(const TcpPacket& packet);
  bool operator==(const CheckCreateUserNameResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CHECK_CREATE_USER_NAME_RESPONSE_H_
