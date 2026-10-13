#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_ADDITIVE_STREAM_CIPHER_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_ADDITIVE_STREAM_CIPHER_H_

#include <cstddef>
#include <cstdint>
#include <span>

namespace bubble_fighter::server::protocol {
enum class WireDirection { kClientToServer, kServerToClient };

class AdditiveStreamCipher {
 public:
  explicit AdditiveStreamCipher(WireDirection direction);

  // Use a separate instance for each connection direction. Indices persist
  // across chunks and packets; the 258-byte greeting is excluded.
  void Encode(std::span<std::uint8_t> bytes);
  void Decode(std::span<std::uint8_t> bytes);

 private:
  void Transform(std::span<std::uint8_t> bytes, bool encode);

  std::size_t first_index_;
  std::size_t second_index_;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_ADDITIVE_STREAM_CIPHER_H_
