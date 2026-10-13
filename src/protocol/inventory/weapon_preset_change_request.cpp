#include "protocol/inventory/weapon_preset_change_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket WeaponPresetChangeRequest::Encode() const {
  if (preset_index > kMaxPresetIndex) {
    throw std::invalid_argument("Invalid weapon preset index.");
  }
  PacketWriter writer;
  writer.WriteUInt32(object_id);
  writer.WriteUInt8(preset_index);
  return {kMessageId, writer.TakeBytes()};
}

WeaponPresetChangeRequest WeaponPresetChangeRequest::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid weapon preset change request.");
  }
  PacketReader reader(packet.payload);
  WeaponPresetChangeRequest result{reader.ReadUInt32(), reader.ReadUInt8()};
  reader.RequireEnd();
  if (result.preset_index > kMaxPresetIndex) {
    throw std::invalid_argument("Invalid weapon preset index.");
  }
  return result;
}
}  // namespace bubble_fighter::server::protocol
