#include "protocol/battle/game_transport_options_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GameTransportOptionsNotification::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(relay_enabled_value);
  writer.WriteUInt32(udp_enabled_value);
  writer.WriteUInt32(movement_delta_enabled_value);
  return {kMessageId, writer.TakeBytes()};
}

GameTransportOptionsNotification GameTransportOptionsNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid game transport options message.");
  }
  PacketReader reader(packet.payload);
  GameTransportOptionsNotification result;
  result.relay_enabled_value = reader.ReadUInt32();
  result.udp_enabled_value = reader.ReadUInt32();
  result.movement_delta_enabled_value = reader.ReadUInt32();
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
