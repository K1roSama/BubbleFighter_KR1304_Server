#include "protocol/login/login_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket LoginRequest::Encode() const {
  if (local_ipv4_addresses.size() > kMaxLocalIpv4Addresses) {
    throw std::length_error("Login address count exceeds supported limit.");
  }
  PacketWriter writer;
  writer.WriteUtf16(nexon_passport);
  writer.WriteUtf16(hardware_id);
  writer.WriteUtf16(unknown_string);
  writer.WriteUInt32(static_cast<std::uint32_t>(local_ipv4_addresses.size()));
  for (const auto& address : local_ipv4_addresses) {
    for (const auto octet : address) {
      writer.WriteUInt8(octet);
    }
  }
  writer.WriteUInt32(resource_value);
  writer.WriteUInt32(unknown_value);
  writer.WriteUtf16(platform_string);
  return {kMessageId, writer.TakeBytes()};
}

LoginRequest LoginRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected login request message id.");
  }
  PacketReader reader(packet.payload);
  LoginRequest message;
  message.nexon_passport = reader.ReadUtf16();
  message.hardware_id = reader.ReadUtf16();
  message.unknown_string = reader.ReadUtf16();
  const auto count = reader.ReadUInt32();
  if (count > kMaxLocalIpv4Addresses) {
    throw std::length_error("Login address count exceeds supported limit.");
  }
  message.local_ipv4_addresses.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    std::array<std::uint8_t, 4> address{};
    for (auto& octet : address) {
      octet = reader.ReadUInt8();
    }
    message.local_ipv4_addresses.push_back(address);
  }
  message.resource_value = reader.ReadUInt32();
  message.unknown_value = reader.ReadUInt32();
  message.platform_string = reader.ReadUtf16();
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
