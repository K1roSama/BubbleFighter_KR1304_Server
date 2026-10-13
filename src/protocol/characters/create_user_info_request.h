#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CREATE_USER_INFO_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CREATE_USER_INFO_REQUEST_H_

#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E028: sent by BFCLIENT02 0x009A4E90 after the name confirmation dialog.
struct CreateUserInfoRequest {
  static constexpr std::uint16_t kMessageId = 0x0020;

  std::u16string character_name;
  // E030: scene CharacterSpec chrType / colorIndex, in this wire order.
  std::uint32_t character_type = 0;
  std::uint32_t color_index = 0;
  std::u16string unknown_string;
  std::uint32_t unknown_value = 0;

  TcpPacket Encode() const;
  static CreateUserInfoRequest Decode(const TcpPacket& packet);
  bool operator==(const CreateUserInfoRequest&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CREATE_USER_INFO_REQUEST_H_
