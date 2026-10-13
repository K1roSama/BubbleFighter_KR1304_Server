#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_ZLIB_STORED_STREAM_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_ZLIB_STORED_STREAM_H_

#include <cstdint>
#include <span>
#include <vector>

namespace bubble_fighter::server::protocol {
// Emits standard zlib with stored DEFLATE blocks. No compression dependency
// is needed for bounded server-owned configuration/inventory records.
class ZlibStoredStream {
 public:
  static constexpr std::size_t kMaxInputBytes = 256 * 1024;
  static std::vector<std::uint8_t> Encode(std::span<const std::uint8_t> bytes);
  static std::uint32_t Adler32(std::span<const std::uint8_t> bytes,
                               std::uint32_t seed);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_ZLIB_STORED_STREAM_H_
