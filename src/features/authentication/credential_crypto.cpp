#include "features/authentication/credential_crypto.h"

#include "crypto/sha256.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <bcrypt.h>

#include <algorithm>
#include <stdexcept>

namespace bubble_fighter::server::features::authentication {
namespace {
void RequireSuccess(NTSTATUS status) {
  if (status < 0) {
    throw std::runtime_error("Credential cryptography operation failed.");
  }
}

class Algorithm {
 public:
  explicit Algorithm(ULONG flags) {
    RequireSuccess(BCryptOpenAlgorithmProvider(
        &handle_, BCRYPT_SHA256_ALGORITHM, nullptr, flags));
  }
  ~Algorithm() { BCryptCloseAlgorithmProvider(handle_, 0); }
  Algorithm(const Algorithm&) = delete;
  Algorithm& operator=(const Algorithm&) = delete;
  BCRYPT_ALG_HANDLE Get() const { return handle_; }

 private:
  BCRYPT_ALG_HANDLE handle_ = nullptr;
};

CredentialDigest Derive(std::string_view password,
                        const PasswordVerifier& verifier) {
  if (password.size() > 1024 || verifier.iterations < 600000 ||
      verifier.iterations > 1000000) {
    throw std::invalid_argument("Unsupported password verifier parameters.");
  }
  Algorithm algorithm(BCRYPT_ALG_HANDLE_HMAC_FLAG);
  CredentialDigest result{};
  RequireSuccess(BCryptDeriveKeyPBKDF2(
      algorithm.Get(),
      reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),
      static_cast<ULONG>(password.size()),
      const_cast<PUCHAR>(verifier.salt.data()),
      static_cast<ULONG>(verifier.salt.size()), verifier.iterations,
      result.data(), static_cast<ULONG>(result.size()), 0));
  return result;
}
}  // namespace

PasswordVerifier CredentialCrypto::HashPassword(std::string_view password) {
  if (password.size() < 12 || password.size() > 1024) {
    throw std::invalid_argument(
        "Local passwords require 12 to 1024 UTF-8 bytes.");
  }
  PasswordVerifier verifier;
  RequireSuccess(BCryptGenRandom(nullptr, verifier.salt.data(),
                                 static_cast<ULONG>(verifier.salt.size()),
                                 BCRYPT_USE_SYSTEM_PREFERRED_RNG));
  verifier.hash = Derive(password, verifier);
  return verifier;
}

bool CredentialCrypto::VerifyPassword(std::string_view password,
                                      const PasswordVerifier& verifier) {
  auto actual = Derive(password, verifier);
  // Fixed-size comparison with no data-dependent early exit.
  volatile std::uint8_t difference = 0;
  for (std::size_t index = 0; index < actual.size(); ++index) {
    difference = static_cast<std::uint8_t>(
        difference | (actual[index] ^ verifier.hash[index]));
  }
  SecureZeroMemory(actual.data(), actual.size());
  return difference == 0;
}

CredentialDigest CredentialCrypto::Sha256(std::span<const std::uint8_t> bytes) {
  if (bytes.size() > 1024) {
    throw std::invalid_argument("Credential digest input is too long.");
  }
  const auto result = crypto::HashSha256(bytes);
  if (result.status != crypto::Sha256Status::kSuccess) {
    throw std::runtime_error("Credential cryptography operation failed.");
  }
  return result.digest;
}

std::string CredentialCrypto::GenerateTicket() {
  CredentialDigest random{};
  RequireSuccess(BCryptGenRandom(nullptr, random.data(),
                                 static_cast<ULONG>(random.size()),
                                 BCRYPT_USE_SYSTEM_PREFERRED_RNG));
  constexpr char digits[] = "0123456789abcdef";
  std::string result = "BFS1.";
  result.reserve(69);
  for (const auto value : random) {
    result.push_back(digits[value >> 4]);
    result.push_back(digits[value & 15]);
  }
  SecureZeroMemory(random.data(), random.size());
  return result;
}

bool CredentialCrypto::IsTicketFormatValid(std::string_view ticket) {
  return ticket.size() == 69 && ticket.starts_with("BFS1.") &&
         std::all_of(ticket.begin() + 5, ticket.end(), [](char value) {
           return (value >= '0' && value <= '9') ||
                  (value >= 'a' && value <= 'f');
         });
}

CredentialDigest CredentialCrypto::HashTicket(std::string_view ticket) {
  if (!IsTicketFormatValid(ticket)) {
    throw std::invalid_argument("Invalid local ticket format.");
  }
  return Sha256(
      {reinterpret_cast<const std::uint8_t*>(ticket.data()), ticket.size()});
}
}  // namespace bubble_fighter::server::features::authentication
