#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_CHAT_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_CHAT_NOTIFICATION_H_
#include <string>

#include "protocol/codec/tcp_packet_codec.h"
namespace bubble_fighter::server::protocol {
// BFCLIENT02 0x10C4A50, dispatched by S509 at 0x9D9930.
// The last UTF-16 string and boolean are absent from the old client format.
struct ChatNotification {
  static constexpr std::uint16_t kMessageId = 509;
  std::int16_t slot = -3;
  std::u16string sender;
  std::uint32_t channel = 0;
  std::uint32_t value_at32 = 0;
  std::u16string expression;  // "e" enables ordinary actor chat in 0x9EAC70.
  std::u16string text;
  std::uint32_t text_color = 0xFFFFFFFF;
  std::uint32_t name_color = 0xFFFFFFFF;
  bool flag_at92 = false;
  std::u16string
      string_at96;  // Optional validated category/motion, e.g. emotion/love.
  bool flag_at120 = false;
  TcpPacket Encode() const;
  static ChatNotification Decode(const TcpPacket& packet);
  bool operator==(const ChatNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_CHAT_NOTIFICATION_H_
