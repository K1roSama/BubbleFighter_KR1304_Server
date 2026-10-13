#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_PACKET_STREAM_DECODER_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_PACKET_STREAM_DECODER_H_

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
class PacketStreamDecoder {
 public:
  explicit PacketStreamDecoder(
      std::uint32_t max_payload_size = TcpPacketCodec::kDefaultMaxPayloadSize);

  // Input is already decrypted. A connection failure poisons this instance.
  std::vector<TcpPacket> Feed(std::span<const std::uint8_t> bytes);
  void Finish();
  bool IsFailed() const;
  bool HasPendingData() const;

 private:
  [[noreturn]] void Fail(const char* message);

  std::uint32_t max_payload_size_;
  std::vector<std::uint8_t> buffer_;
  bool failed_ = false;
  bool finished_ = false;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_PACKET_STREAM_DECODER_H_
