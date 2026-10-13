#include "protocol/configuration/dynamic_config_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket DynamicConfigResponse::Encode() const {
  std::uint32_t mask = 0;
  std::size_t total = 0;
  for (std::size_t index = 0; index < records.size(); ++index) {
    if (!records[index]) {
      continue;
    }
    const auto size = records[index]->size();
    if (size == 0 || size > kMaxCompressedBytes - total) {
      throw std::invalid_argument("Invalid compressed configuration size.");
    }
    total += size;
    mask |= std::uint32_t{1} << index;
  }
  PacketWriter writer;
  writer.WriteUInt32(mask);
  for (const auto& record : records) {
    if (!record) {
      continue;
    }
    writer.WriteUInt32(static_cast<std::uint32_t>(record->size()));
    for (const auto byte : *record) {
      writer.WriteUInt8(byte);
    }
  }
  return {kMessageId, writer.TakeBytes()};
}

DynamicConfigResponse DynamicConfigResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument(
        "Unexpected dynamic configuration response id.");
  }
  PacketReader reader(packet.payload);
  const auto mask = reader.ReadUInt32();
  DynamicConfigResponse response;
  std::size_t total = 0;
  for (std::size_t index = 0; index < response.records.size(); ++index) {
    if (!(mask & (std::uint32_t{1} << index))) {
      continue;
    }
    const auto count = reader.ReadUInt32();
    if (count == 0 || count > kMaxCompressedBytes - total ||
        count > reader.RemainingBytes()) {
      throw std::invalid_argument("Invalid compressed configuration length.");
    }
    total += count;
    auto& record = response.records[index].emplace();
    record.reserve(count);
    for (std::uint32_t item = 0; item < count; ++item) {
      record.push_back(reader.ReadUInt8());
    }
  }
  reader.RequireEnd();
  return response;
}
}  // namespace bubble_fighter::server::protocol
