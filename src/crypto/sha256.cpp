#include "crypto/sha256.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <bcrypt.h>

namespace bubble_fighter::server::crypto {

Sha256Result HashSha256(std::span<const std::uint8_t> bytes) {
  Sha256Result result;
  BCRYPT_ALG_HANDLE algorithm = nullptr;
  if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
                                  0) < 0) {
    return result;
  }
  const auto status =
      BCryptHash(algorithm, nullptr, 0, const_cast<PUCHAR>(bytes.data()),
                 static_cast<ULONG>(bytes.size()), result.digest.data(),
                 static_cast<ULONG>(result.digest.size()));
  BCryptCloseAlgorithmProvider(algorithm, 0);
  result.status =
      status < 0 ? Sha256Status::kHashFailed : Sha256Status::kSuccess;
  return result;
}

}  // namespace bubble_fighter::server::crypto
