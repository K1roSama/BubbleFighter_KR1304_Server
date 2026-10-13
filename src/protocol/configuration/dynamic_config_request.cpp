#include "protocol/configuration/dynamic_config_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket DynamicConfigRequest::Encode() const {
  std::uint32_t mask = 0;
  for (std::size_t index = 0; index < checksums.size(); ++index) {
    if (checksums[index]) {
      mask |= std::uint32_t{1} << index;
    }
  }
  PacketWriter writer;
  writer.WriteUInt32(mask);
  for (const auto& checksum : checksums) {
    if (checksum) {
      writer.WriteUInt32(*checksum);
    }
  }
  return {kMessageId, writer.TakeBytes()};
}

DynamicConfigRequest DynamicConfigRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected dynamic configuration request id.");
  }
  PacketReader reader(packet.payload);
  const auto mask = reader.ReadUInt32();
  DynamicConfigRequest request;
  for (std::size_t index = 0; index < request.checksums.size(); ++index) {
    if (mask & (std::uint32_t{1} << index)) {
      request.checksums[index] = reader.ReadUInt32();
    }
  }
  reader.RequireEnd();
  return request;
}
}  // namespace bubble_fighter::server::protocol
