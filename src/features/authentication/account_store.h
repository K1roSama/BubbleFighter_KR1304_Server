#ifndef BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_ACCOUNT_STORE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_ACCOUNT_STORE_H_

#include <optional>

#include "features/authentication/credential_crypto.h"

namespace bubble_fighter::server::features::authentication {
struct StoredAccount {
  std::uint64_t account_id = 0;
  PasswordVerifier password;
  bool is_disabled = false;
};

class AccountStore {
 public:
  virtual ~AccountStore() = default;
  virtual bool CreateAccount(std::string_view normalized_username,
                             const PasswordVerifier& password) = 0;
  virtual std::optional<StoredAccount> FindAccount(
      std::string_view normalized_username) = 0;
  virtual bool AddTicket(std::uint64_t account_id,
                         const CredentialDigest& digest,
                         std::uint32_t lifetime_seconds) = 0;
  // Must atomically reject expired, consumed and disabled-account tickets.
  virtual std::optional<std::uint64_t> ConsumeTicket(
      const CredentialDigest& digest) = 0;
};
}  // namespace bubble_fighter::server::features::authentication

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_ACCOUNT_STORE_H_
