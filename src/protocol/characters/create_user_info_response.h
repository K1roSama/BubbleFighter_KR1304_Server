#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CREATE_USER_INFO_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CREATE_USER_INFO_RESPONSE_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E028: gspRepCreateUserInfo, read at BFCLIENT02 0x009A46A0.
struct CreateUserInfoResponse {
  static constexpr std::uint16_t kMessageId = 0x000E;

  std::uint32_t result = 0;

  TcpPacket Encode() const;
  static CreateUserInfoResponse Decode(const TcpPacket& packet);
  bool operator==(const CreateUserInfoResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CREATE_USER_INFO_RESPONSE_H_
