#include "protocol/scenes/goto_lobby_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GotoLobbyNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(party_move_notice);
  return {kMessageId, writer.TakeBytes()};
}

GotoLobbyNotification GotoLobbyNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected goto-lobby id.");
  }
  PacketReader reader(packet.payload);
  GotoLobbyNotification notification{reader.ReadUInt32()};
  reader.RequireEnd();
  return notification;
}
}  // namespace bubble_fighter::server::protocol
