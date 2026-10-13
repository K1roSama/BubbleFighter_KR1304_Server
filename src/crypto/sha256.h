#ifndef BUBBLEFIGHTER_SERVER_CRYPTO_SHA256_H_
#define BUBBLEFIGHTER_SERVER_CRYPTO_SHA256_H_

#include <array>
#include <cstdint>
#include <span>

namespace bubble_fighter::server::crypto {

enum class Sha256Status { kSuccess, kInitializationFailed, kHashFailed };
struct Sha256Result {
  Sha256Status status = Sha256Status::kInitializationFailed;
  std::array<std::uint8_t, 32> digest{};
};

// The caller owns its input limit and exception text. Bytes must fit ULONG.
Sha256Result HashSha256(std::span<const std::uint8_t> bytes);

}  // namespace bubble_fighter::server::crypto

#endif  // BUBBLEFIGHTER_SERVER_CRYPTO_SHA256_H_
