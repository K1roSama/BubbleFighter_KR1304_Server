#include "protocol/codec/packet_stream_decoder.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace bubble_fighter::server::protocol {
PacketStreamDecoder::PacketStreamDecoder(std::uint32_t max_payload_size)
    : max_payload_size_(max_payload_size) {}

std::vector<TcpPacket> PacketStreamDecoder::Feed(
    std::span<const std::uint8_t> bytes) {
  if (failed_) {
    throw std::runtime_error("Packet stream has already failed.");
  }
  if (finished_) {
    throw std::runtime_error("Packet stream has already ended.");
  }
  std::vector<TcpPacket> packets;
  for (;;) {
    auto result = TcpPacketCodec::Decode(buffer_, max_payload_size_);
    switch (result.status) {
      case PacketDecodeStatus::kComplete:
        packets.push_back(std::move(result.packet));
        buffer_.clear();
        continue;
      case PacketDecodeStatus::kInvalidHeaderChecksum:
        Fail("Invalid packet header checksum.");
      case PacketDecodeStatus::kInvalidPayloadChecksum:
        Fail("Invalid packet payload checksum.");
      case PacketDecodeStatus::kPayloadTooLarge:
        Fail("Packet payload exceeds configured limit.");
      case PacketDecodeStatus::kNeedMoreData:
        break;
    }
    if (bytes.empty()) {
      return packets;
    }
    // Keep at most one incomplete frame. Validate the header before
    // buffering a payload, including when the input contains many frames.
    const auto count =
        std::min(result.frame_size - buffer_.size(), bytes.size());
    buffer_.insert(buffer_.end(), bytes.begin(), bytes.begin() + count);
    bytes = bytes.subspan(count);
  }
}

void PacketStreamDecoder::Finish() {
  if (failed_) {
    throw std::runtime_error("Packet stream has already failed.");
  }
  if (!buffer_.empty()) {
    Fail("Connection ended with a truncated packet.");
  }
  finished_ = true;
}

bool PacketStreamDecoder::IsFailed() const { return failed_; }

bool PacketStreamDecoder::HasPendingData() const { return !buffer_.empty(); }

void PacketStreamDecoder::Fail(const char* message) {
  failed_ = true;
  buffer_.clear();
  throw std::runtime_error(message);
}
}  // namespace bubble_fighter::server::protocol
