#include "persistence/my_sql_character_store.h"

#include <array>

#include "features/characters/character_creation.h"
#include "persistence/my_sql_commands.h"
#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::persistence {
using namespace features::characters;
using namespace my_sql_commands;

namespace {
std::vector<std::uint8_t> NameBytes(std::u16string_view name) {
  if (CharacterCreation::ValidateName(name) != 0) {
    throw std::invalid_argument("Invalid character name.");
  }
  protocol::PacketWriter writer;
  for (const auto value : name) {
    writer.WriteUInt16(value);
  }
  return writer.TakeBytes();
}
}  // namespace

MySqlCharacterStore::MySqlCharacterStore(
    const std::filesystem::path& configuration)
    : connection_(configuration) {}

std::optional<CharacterRecord> MySqlCharacterStore::FindByAccount(
    std::uint64_t account_id) {
  // A database missing the statistics migration fails here; never silently
  // turn an existing role into a new character or invent missing values.
  Statement query(
      connection_.Handle(),
      "SELECT CharacterId, NameUtf16, CharacterType, ColorIndex, Level, Experience, Lucci, Cash FROM Characters WHERE AccountId = ?");
  std::array parameters{Number(account_id)};
  query.Execute(parameters);
  CharacterRecord record;
  record.account_id = account_id;
  std::array<std::uint8_t, CharacterCreation::kMaxNameCodeUnits * 2> name{};
  unsigned long length = 0;
  std::array results{
      Number(record.character_id),
      Bytes(name.data(), static_cast<unsigned long>(name.size())),
      Number(record.character_type),
      Number(record.color_index),
      Number(record.statistics.level),
      Number(record.statistics.experience),
      Number(record.statistics.lucci),
      Number(record.statistics.cash)};
  results[1].length = &length;
  if (!query.Fetch(results)) {
    return {};
  }
  if (length == 0 || length > name.size() || length % 2 != 0 ||
      record.character_id == 0) {
    throw std::runtime_error("Invalid stored character.");
  }
  protocol::PacketReader reader(std::span(name).first(length));
  while (reader.RemainingBytes()) {
    record.character_name.push_back(reader.ReadUInt16());
  }
  if (CharacterCreation::ValidateName(record.character_name) ||
      !CharacterCreation::SupportsSelection(record.character_type,
                                            record.color_index) ||
      !record.statistics.IsValid()) {
    throw std::runtime_error("Unsupported stored character fields.");
  }
  return record;
}

bool MySqlCharacterStore::NameExists(std::u16string_view name) {
  const auto bytes = NameBytes(name);
  Statement query(connection_.Handle(),
                  "SELECT CharacterId FROM Characters WHERE NameUtf16 = ?");
  std::array parameters{
      Bytes(bytes.data(), static_cast<unsigned long>(bytes.size()))};
  query.Execute(parameters);
  std::uint64_t id = 0;
  std::array results{Number(id)};
  return query.Fetch(results);
}

CharacterCreateResult MySqlCharacterStore::Create(CharacterRecord& character) {
  const auto name = NameBytes(character.character_name);
  if (!CharacterCreation::SupportsSelection(character.character_type,
                                            character.color_index)) {
    throw std::invalid_argument("Unsupported character selection.");
  }
  auto* connection = connection_.Handle();
  Transaction transaction(connection);
  {
    // Serializes creation for the same account; unique name protects
    // cross-account races even when both earlier name checks passed.
    Statement account(
        connection,
        "SELECT AccountId FROM Accounts WHERE AccountId = ? AND IsDisabled = 0 FOR UPDATE");
    std::array parameters{Number(character.account_id)};
    account.Execute(parameters);
    std::uint64_t id = 0;
    std::array results{Number(id)};
    if (!account.Fetch(results)) {
      return CharacterCreateResult::kAccountUnavailable;
    }
  }
  if (FindByAccount(character.account_id)) {
    return CharacterCreateResult::kAccountHasCharacter;
  }
  const CharacterStatistics initial_statistics;
  Statement insert(
      connection,
      "INSERT INTO Characters (AccountId, NameUtf16, CharacterType, ColorIndex, Level, Experience, Lucci, Cash) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
  std::array parameters{
      Number(character.account_id),
      Bytes(name.data(), static_cast<unsigned long>(name.size())),
      Number(character.character_type),
      Number(character.color_index),
      Number(initial_statistics.level),
      Number(initial_statistics.experience),
      Number(initial_statistics.lucci),
      Number(initial_statistics.cash)};
  try {
    insert.Execute(parameters);
  } catch (const MySqlError& error) {
    if (error.Code() == 1062) {
      return CharacterCreateResult::kNameTaken;
    }
    throw;
  }
  if (insert.AffectedRows() != 1) {
    throw std::runtime_error("Character insert did not affect one row.");
  }
  const auto character_id = mysql_insert_id(connection);
  transaction.Commit();
  // Statistics use the same local policy as the migration, never values
  // supplied to Create. Publish identity and statistics only on commit.
  character.character_id = character_id;
  character.statistics = initial_statistics;
  return CharacterCreateResult::kCreated;
}
}  // namespace bubble_fighter::server::persistence
