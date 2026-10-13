#ifndef BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_CREDENTIAL_CRYPTO_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_CREDENTIAL_CRYPTO_H_

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace bubble_fighter::server::features::authentication {
using CredentialDigest = std::array<std::uint8_t, 32>;

struct PasswordVerifier {
  std::array<std::uint8_t, 16> salt{};
  CredentialDigest hash{};
  std::uint32_t iterations = 600000;
};

class CredentialCrypto {
 public:
  static PasswordVerifier HashPassword(std::string_view password);
  static bool VerifyPassword(std::string_view password,
                             const PasswordVerifier& verifier);
  static CredentialDigest Sha256(std::span<const std::uint8_t> bytes);
  static std::string GenerateTicket();
  static bool IsTicketFormatValid(std::string_view ticket);
  static CredentialDigest HashTicket(std::string_view ticket);
};
}  // namespace bubble_fighter::server::features::authentication

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_CREDENTIAL_CRYPTO_H_
