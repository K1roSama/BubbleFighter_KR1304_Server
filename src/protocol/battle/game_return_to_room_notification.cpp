#include "protocol/battle/game_return_to_room_notification.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
TcpPacket GameReturnToRoomNotification::Encode() const {
  return {kMessageId, {}};
}

GameReturnToRoomNotification GameReturnToRoomNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId || !packet.payload.empty()) {
    throw std::invalid_argument("Invalid return-to-room notification.");
  }
  return {};
}
}  // namespace bubble_fighter::server::protocol
