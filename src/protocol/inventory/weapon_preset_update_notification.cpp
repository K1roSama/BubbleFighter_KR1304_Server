#include "protocol/inventory/weapon_preset_update_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket WeaponPresetUpdateNotification::Encode() const {
  if (records.size() > kMaxCollectionEntries ||
      bytes.size() > kMaxCollectionEntries) {
    throw std::invalid_argument(
        "Weapon preset collection exceeds local limit.");
  }
  PacketWriter writer;
  writer.WriteUInt8(byte_at0);
  writer.WriteUInt32(player_id);
  writer.WriteUInt32(static_cast<std::uint32_t>(records.size()));
  for (const auto& record : records) {
    for (const auto value : record) {
      writer.WriteUInt32(value);
    }
  }
  writer.WriteUInt32(static_cast<std::uint32_t>(bytes.size()));
  for (const auto value : bytes) {
    writer.WriteUInt8(value);
  }
  return {kMessageId, writer.TakeBytes()};
}

WeaponPresetUpdateNotification WeaponPresetUpdateNotification::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected weapon preset notification id.");
  }
  PacketReader reader(packet.payload);
  WeaponPresetUpdateNotification result;
  result.byte_at0 = reader.ReadUInt8();
  result.player_id = reader.ReadUInt32();
  const auto count = reader.ReadUInt32();
  if (count > kMaxCollectionEntries || count > reader.RemainingBytes() / 12) {
    throw std::invalid_argument("Invalid weapon preset count.");
  }
  result.records.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    result.records.push_back(
        {reader.ReadUInt32(), reader.ReadUInt32(), reader.ReadUInt32()});
  }
  const auto byte_count = reader.ReadUInt32();
  if (byte_count > kMaxCollectionEntries ||
      byte_count > reader.RemainingBytes()) {
    throw std::invalid_argument("Invalid weapon preset byte count.");
  }
  result.bytes.reserve(byte_count);
  for (std::uint32_t index = 0; index < byte_count; ++index) {
    result.bytes.push_back(reader.ReadUInt8());
  }
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
