#ifndef BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_CHARACTER_STORE_H_
#define BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_CHARACTER_STORE_H_

#include "features/characters/character_store.h"
#include "persistence/my_sql_connection.h"

namespace bubble_fighter::server::persistence {
class MySqlCharacterStore final : public features::characters::CharacterStore {
 public:
  explicit MySqlCharacterStore(const std::filesystem::path& configuration);
  std::optional<features::characters::CharacterRecord> FindByAccount(
      std::uint64_t account_id) override;
  bool NameExists(std::u16string_view name) override;
  features::characters::CharacterCreateResult Create(
      features::characters::CharacterRecord& character) override;

 private:
  MySqlConnection connection_;
};
}  // namespace bubble_fighter::server::persistence

#endif  // BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_CHARACTER_STORE_H_
