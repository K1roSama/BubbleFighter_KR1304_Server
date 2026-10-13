#include "protocol/login/center_list_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket CenterListNotification::Encode() const {
  if (gates.size() > kMaxEntries) {
    throw std::invalid_argument("Center list exceeds local entry limit.");
  }
  PacketWriter writer;
  writer.WriteUInt32(static_cast<std::uint32_t>(gates.size()));
  for (const auto& gate : gates) {
    writer.WriteUInt32(gate.unknown_value_at4);
    writer.WriteUInt32(gate.unknown_value_at8);
    writer.WriteUInt32(gate.unknown_value_at12);
    for (const auto byte : gate.ipv4_address) {
      writer.WriteUInt8(byte);
    }
    writer.WriteUInt16(gate.port);
  }
  return {kMessageId, writer.TakeBytes()};
}

CenterListNotification CenterListNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected center list message id.");
  }
  PacketReader reader(packet.payload);
  const auto count = reader.ReadUInt32();
  // GateInfo's x86 object is 24 bytes; its wire representation is 18.
  if (count > kMaxEntries || count > reader.RemainingBytes() / 18) {
    throw std::invalid_argument("Invalid center list entry count.");
  }
  CenterListNotification message;
  message.gates.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    CenterGateInfo gate;
    gate.unknown_value_at4 = reader.ReadUInt32();
    gate.unknown_value_at8 = reader.ReadUInt32();
    gate.unknown_value_at12 = reader.ReadUInt32();
    for (auto& byte : gate.ipv4_address) {
      byte = reader.ReadUInt8();
    }
    gate.port = reader.ReadUInt16();
    message.gates.push_back(gate);
  }
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
