#include "persistence/my_sql_account_store.h"

#include <mysql.h>

#include <array>
#include <span>

#include "persistence/my_sql_commands.h"

namespace bubble_fighter::server::persistence {
using namespace features::authentication;
using namespace my_sql_commands;

MySqlAccountStore::MySqlAccountStore(
    const std::filesystem::path& configuration_file)
    : connection_(configuration_file) {}

bool MySqlAccountStore::CreateAccount(std::string_view username,
                                      const PasswordVerifier& password) {
  if (username.size() > 32) {
    throw std::invalid_argument("Account username is too long.");
  }
  Statement statement(
      connection_.Handle(),
      "INSERT INTO Accounts (Username, PasswordSalt, PasswordHash, PasswordIterations) VALUES (?, ?, ?, ?)");
  std::array parameters{
      Bytes(username.data(), static_cast<unsigned long>(username.size()),
            MYSQL_TYPE_STRING),
      Bytes(password.salt.data(),
            static_cast<unsigned long>(password.salt.size())),
      Bytes(password.hash.data(),
            static_cast<unsigned long>(password.hash.size())),
      Number(password.iterations)};
  try {
    statement.Execute(parameters);
  } catch (const MySqlError& error) {
    if (error.Code() == 1062) {
      return false;
    }
    throw;
  }
  return statement.AffectedRows() == 1;
}

std::optional<StoredAccount> MySqlAccountStore::FindAccount(
    std::string_view username) {
  if (username.size() > 32) {
    return std::nullopt;
  }
  Statement statement(
      connection_.Handle(),
      "SELECT AccountId, PasswordSalt, PasswordHash, PasswordIterations, IsDisabled FROM Accounts WHERE Username = ?");
  std::array parameters{Bytes(username.data(),
                              static_cast<unsigned long>(username.size()),
                              MYSQL_TYPE_STRING)};
  statement.Execute(parameters);
  StoredAccount account;
  std::uint8_t disabled = 0;
  unsigned long salt_length = 0;
  unsigned long hash_length = 0;
  std::array results{
      Number(account.account_id),
      Bytes(account.password.salt.data(),
            static_cast<unsigned long>(account.password.salt.size())),
      Bytes(account.password.hash.data(),
            static_cast<unsigned long>(account.password.hash.size())),
      Number(account.password.iterations), Number(disabled)};
  results[1].length = &salt_length;
  results[2].length = &hash_length;
  if (!statement.Fetch(results)) {
    return std::nullopt;
  }
  if (salt_length != account.password.salt.size() ||
      hash_length != account.password.hash.size() || disabled > 1 ||
      account.account_id == 0) {
    throw std::runtime_error("Invalid stored account fields.");
  }
  account.is_disabled = disabled != 0;
  return account;
}

bool MySqlAccountStore::AddTicket(std::uint64_t account_id,
                                  const CredentialDigest& digest,
                                  std::uint32_t lifetime_seconds) {
  if (lifetime_seconds == 0 || lifetime_seconds > 300) {
    throw std::invalid_argument("Unsupported ticket lifetime.");
  }
  Statement statement(
      connection_.Handle(),
      "INSERT INTO LoginTickets (TicketHash, AccountId, ExpiresAt) "
      "SELECT ?, AccountId, TIMESTAMPADD(SECOND, ?, UTC_TIMESTAMP(6)) FROM Accounts WHERE AccountId = ? AND IsDisabled = 0");
  std::array parameters{
      Bytes(digest.data(), static_cast<unsigned long>(digest.size())),
      Number(lifetime_seconds), Number(account_id)};
  statement.Execute(parameters);
  return statement.AffectedRows() == 1;
}

std::optional<std::uint64_t> MySqlAccountStore::ConsumeTicket(
    const CredentialDigest& digest) {
  auto* connection = connection_.Handle();
  Transaction transaction(connection);
  std::uint64_t account_id = 0;
  std::array parameters{
      Bytes(digest.data(), static_cast<unsigned long>(digest.size()))};
  {
    Statement query(
        connection,
        "SELECT t.AccountId FROM LoginTickets t JOIN Accounts a ON a.AccountId = t.AccountId "
        "WHERE t.TicketHash = ? AND t.ConsumedAt IS NULL AND t.ExpiresAt > UTC_TIMESTAMP(6) AND a.IsDisabled = 0 FOR UPDATE");
    query.Execute(parameters);
    std::array results{Number(account_id)};
    if (!query.Fetch(results)) {
      return std::nullopt;
    }
  }
  Statement update(
      connection,
      "UPDATE LoginTickets SET ConsumedAt = UTC_TIMESTAMP(6) "
      "WHERE TicketHash = ? AND ConsumedAt IS NULL AND ExpiresAt > UTC_TIMESTAMP(6)");
  update.Execute(parameters);
  if (update.AffectedRows() != 1) {
    return std::nullopt;
  }
  transaction.Commit();
  return account_id;
}
}  // namespace bubble_fighter::server::persistence
