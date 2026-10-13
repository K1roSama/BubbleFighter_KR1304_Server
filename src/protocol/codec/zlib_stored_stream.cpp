#include "protocol/codec/zlib_stored_stream.h"

#include <algorithm>
#include <stdexcept>

namespace bubble_fighter::server::protocol {
std::uint32_t ZlibStoredStream::Adler32(std::span<const std::uint8_t> bytes,
                                        std::uint32_t seed) {
  std::uint32_t low = seed & 0xFFFF;
  std::uint32_t high = seed >> 16;
  for (const auto byte : bytes) {
    low = (low + byte) % 65521;
    high = (high + low) % 65521;
  }
  return (high << 16) | low;
}

std::vector<std::uint8_t> ZlibStoredStream::Encode(
    std::span<const std::uint8_t> bytes) {
  if (bytes.size() > kMaxInputBytes) {
    throw std::invalid_argument("Data exceeds local zlib input limit.");
  }
  std::vector<std::uint8_t> result{0x78, 0x01};
  std::size_t position = 0;
  do {
    const auto count = static_cast<std::uint16_t>(
        std::min<std::size_t>(65535, bytes.size() - position));
    const auto complement = static_cast<std::uint16_t>(~count);
    result.push_back(position + count == bytes.size() ? 1 : 0);
    result.push_back(static_cast<std::uint8_t>(count));
    result.push_back(static_cast<std::uint8_t>(count >> 8));
    result.push_back(static_cast<std::uint8_t>(complement));
    result.push_back(static_cast<std::uint8_t>(complement >> 8));
    result.insert(result.end(), bytes.begin() + position,
                  bytes.begin() + position + count);
    position += count;
  } while (position < bytes.size());
  const auto checksum =
      Adler32(bytes, 1);  // zlib trailer uses the standard seed.
  for (const auto shift : {24, 16, 8, 0}) {
    result.push_back(static_cast<std::uint8_t>(checksum >> shift));
  }
  return result;
}
}  // namespace bubble_fighter::server::protocol
