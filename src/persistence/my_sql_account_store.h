#ifndef BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_ACCOUNT_STORE_H_
#define BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_ACCOUNT_STORE_H_

#include "features/authentication/account_store.h"
#include "persistence/my_sql_connection.h"

namespace bubble_fighter::server::persistence {
class MySqlAccountStore final : public features::authentication::AccountStore {
 public:
  explicit MySqlAccountStore(const std::filesystem::path& configuration_file);
  bool CreateAccount(
      std::string_view normalized_username,
      const features::authentication::PasswordVerifier& password) override;
  std::optional<features::authentication::StoredAccount> FindAccount(
      std::string_view normalized_username) override;
  bool AddTicket(std::uint64_t account_id,
                 const features::authentication::CredentialDigest& digest,
                 std::uint32_t lifetime_seconds) override;
  std::optional<std::uint64_t> ConsumeTicket(
      const features::authentication::CredentialDigest& digest) override;

 private:
  MySqlConnection connection_;
};
}  // namespace bubble_fighter::server::persistence

#endif  // BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_ACCOUNT_STORE_H_
