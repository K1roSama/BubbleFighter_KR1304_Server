#include "features/authentication/account_authentication.h"

#include <stdexcept>

namespace bubble_fighter::server::features::authentication {
AccountAuthentication::AccountAuthentication(AccountStore& store)
    : store_(store) {}

std::string AccountAuthentication::NormalizeUsername(
    std::string_view username) {
  if (username.size() < 3 || username.size() > 32) {
    throw std::invalid_argument(
        "Local usernames require 3 to 32 ASCII letters, digits or underscores.");
  }
  std::string normalized;
  normalized.reserve(username.size());
  for (auto value : username) {
    if (value >= 'A' && value <= 'Z') {
      value = static_cast<char>(value + ('a' - 'A'));
    }
    if (!((value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') ||
          value == '_')) {
      throw std::invalid_argument("Unsupported character in local username.");
    }
    normalized.push_back(value);
  }
  return normalized;
}

bool AccountAuthentication::CreateAccount(std::string_view username,
                                          std::string_view password) {
  const auto normalized = NormalizeUsername(username);
  return store_.CreateAccount(normalized,
                              CredentialCrypto::HashPassword(password));
}

std::optional<std::string> AccountAuthentication::Login(
    std::string_view username, std::string_view password) {
  std::string normalized;
  try {
    normalized = NormalizeUsername(username);
  } catch (const std::invalid_argument&) {
    return std::nullopt;
  }
  if (password.size() > 1024) {
    return std::nullopt;
  }
  const auto account = store_.FindAccount(normalized);
  // Unknown and disabled accounts still perform the same password KDF.
  const PasswordVerifier dummy{};
  const bool valid = CredentialCrypto::VerifyPassword(
      password, account ? account->password : dummy);
  if (!account || !valid || account->is_disabled) {
    return std::nullopt;
  }
  auto ticket = CredentialCrypto::GenerateTicket();
  if (!store_.AddTicket(account->account_id,
                        CredentialCrypto::HashTicket(ticket),
                        kTicketLifetimeSeconds)) {
    return std::nullopt;
  }
  return ticket;
}

std::optional<std::uint64_t> AccountAuthentication::ConsumeTicket(
    std::string_view ticket) {
  if (!CredentialCrypto::IsTicketFormatValid(ticket)) {
    return std::nullopt;
  }
  return store_.ConsumeTicket(CredentialCrypto::HashTicket(ticket));
}
}  // namespace bubble_fighter::server::features::authentication
