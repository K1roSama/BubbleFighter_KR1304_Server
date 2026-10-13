#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CHECK_CREATE_USER_NAME_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CHECK_CREATE_USER_NAME_REQUEST_H_

#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E028: name-check UI action at BFCLIENT02 0x005CCC60.
struct CheckCreateUserNameRequest {
  static constexpr std::uint16_t kMessageId = 0x001F;

  std::u16string character_name;

  TcpPacket Encode() const;
  static CheckCreateUserNameRequest Decode(const TcpPacket& packet);
  bool operator==(const CheckCreateUserNameRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CHECK_CREATE_USER_NAME_REQUEST_H_
