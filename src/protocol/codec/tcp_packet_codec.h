#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_TCP_PACKET_CODEC_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_TCP_PACKET_CODEC_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace bubble_fighter::server::protocol {
struct TcpPacket {
  std::uint16_t message_id = 0;
  std::vector<std::uint8_t> payload;
};

enum class PacketDecodeStatus {
  kComplete,
  kNeedMoreData,
  kInvalidHeaderChecksum,
  kInvalidPayloadChecksum,
  kPayloadTooLarge
};

struct PacketDecodeResult {
  PacketDecodeStatus status = PacketDecodeStatus::kNeedMoreData;
  std::size_t frame_size = 8;
  TcpPacket packet;
};

class TcpPacketCodec {
 public:
  static constexpr std::size_t kHeaderSize = 8;
  // A server resource policy, not an inferred original protocol limit.
  static constexpr std::uint32_t kDefaultMaxPayloadSize = 1024 * 1024;

  static std::vector<std::uint8_t> Encode(
      const TcpPacket& packet,
      std::uint32_t max_payload_size = kDefaultMaxPayloadSize);
  static PacketDecodeResult Decode(
      std::span<const std::uint8_t> bytes,
      std::uint32_t max_payload_size = kDefaultMaxPayloadSize);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_TCP_PACKET_CODEC_H_
