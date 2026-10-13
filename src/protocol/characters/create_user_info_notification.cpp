#include "protocol/characters/create_user_info_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"

namespace bubble_fighter::server::protocol {
TcpPacket CreateUserInfoNotification::Encode() const {
  return {kMessageId, {}};
}

CreateUserInfoNotification CreateUserInfoNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument(
        "Unexpected create-user notification message id.");
  }
  PacketReader{packet.payload}.RequireEnd();
  return {};
}
}  // namespace bubble_fighter::server::protocol
