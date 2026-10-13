#include "protocol/codec/tcp_packet_codec.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace bubble_fighter::server::protocol {
namespace {
std::uint8_t XorBytes(std::span<const std::uint8_t> bytes) {
  std::uint8_t result = 0;
  for (const auto value : bytes) {
    result ^= value;
  }
  return result;
}
}  // namespace

std::vector<std::uint8_t> TcpPacketCodec::Encode(
    const TcpPacket& packet, std::uint32_t max_payload_size) {
  if (packet.payload.size() > max_payload_size ||
      packet.payload.size() > std::numeric_limits<std::uint32_t>::max() ||
      packet.payload.size() >
          std::numeric_limits<std::size_t>::max() - kHeaderSize) {
    throw std::length_error("Packet payload exceeds configured limit.");
  }

  // E005: sub_10BB3E0 writes payload length and XOR checksums.
  std::vector<std::uint8_t> bytes(kHeaderSize + packet.payload.size());
  const auto length = static_cast<std::uint32_t>(packet.payload.size());
  for (std::size_t index = 0; index < 4; ++index) {
    bytes[index] = static_cast<std::uint8_t>(length >> (index * 8));
  }
  bytes[4] = static_cast<std::uint8_t>(packet.message_id);
  bytes[5] = static_cast<std::uint8_t>(packet.message_id >> 8);
  bytes[6] = XorBytes(packet.payload);
  bytes[7] = XorBytes(std::span(bytes).first(7));
  std::copy(packet.payload.begin(), packet.payload.end(),
            bytes.begin() + kHeaderSize);
  return bytes;
}

PacketDecodeResult TcpPacketCodec::Decode(std::span<const std::uint8_t> bytes,
                                          std::uint32_t max_payload_size) {
  PacketDecodeResult result;
  if (bytes.size() < kHeaderSize) {
    return result;
  }
  if (XorBytes(bytes.first(kHeaderSize)) != 0) {
    result.status = PacketDecodeStatus::kInvalidHeaderChecksum;
    return result;
  }

  std::uint32_t length = 0;
  for (std::size_t index = 0; index < 4; ++index) {
    length |= static_cast<std::uint32_t>(bytes[index]) << (index * 8);
  }
  if (length > max_payload_size ||
      length > std::numeric_limits<std::size_t>::max() - kHeaderSize) {
    result.status = PacketDecodeStatus::kPayloadTooLarge;
    return result;
  }
  result.frame_size = kHeaderSize + static_cast<std::size_t>(length);
  if (bytes.size() < result.frame_size) {
    return result;
  }
  const auto payload = bytes.subspan(kHeaderSize, length);
  if (XorBytes(payload) != bytes[6]) {
    result.status = PacketDecodeStatus::kInvalidPayloadChecksum;
    return result;
  }
  result.packet.message_id =
      static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[4]) |
                                 (static_cast<std::uint16_t>(bytes[5]) << 8));
  result.packet.payload.assign(payload.begin(), payload.end());
  result.status = PacketDecodeStatus::kComplete;
  return result;
}
}  // namespace bubble_fighter::server::protocol
