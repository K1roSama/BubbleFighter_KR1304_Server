#include "protocol/inventory/weapon_preset_change_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket WeaponPresetChangeResponse::Encode() const {
  PacketWriter writer;
  writer.WriteUInt8(preset_index);
  writer.WriteUInt32(result);
  return {kMessageId, writer.TakeBytes()};
}

WeaponPresetChangeResponse WeaponPresetChangeResponse::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid weapon preset change response.");
  }
  PacketReader reader(packet.payload);
  WeaponPresetChangeResponse result{reader.ReadUInt8(), reader.ReadUInt32()};
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
