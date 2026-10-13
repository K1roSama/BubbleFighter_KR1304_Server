#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_TRANSPORT_OPTIONS_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_TRANSPORT_OPTIONS_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E082: 0x009E24B5 reads three uint32 values and applies nonzero tests.
// These are transport choices, not authentication or gameplay authority.
struct GameTransportOptionsNotification {
  static constexpr std::uint16_t kMessageId = 585;
  std::uint32_t relay_enabled_value = 0;
  std::uint32_t udp_enabled_value = 0;
  std::uint32_t movement_delta_enabled_value = 0;

  TcpPacket Encode() const;
  static GameTransportOptionsNotification Decode(const TcpPacket& packet);
  bool operator==(const GameTransportOptionsNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_GAME_TRANSPORT_OPTIONS_NOTIFICATION_H_
