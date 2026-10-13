#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CREATE_USER_INFO_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CREATE_USER_INFO_NOTIFICATION_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E031: gspNotifyCreateUserInfo, handled without reading a payload.
struct CreateUserInfoNotification {
  static constexpr std::uint16_t kMessageId = 0x000C;
  TcpPacket Encode() const;
  static CreateUserInfoNotification Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CHARACTERS_CREATE_USER_INFO_NOTIFICATION_H_
