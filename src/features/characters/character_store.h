#ifndef BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_STORE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_STORE_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "features/characters/character_statistics.h"

namespace bubble_fighter::server::features::characters {
struct CharacterRecord {
  std::uint64_t character_id = 0;
  std::uint64_t account_id = 0;
  std::u16string character_name;
  std::uint32_t character_type = 0;
  std::uint32_t color_index = 0;
  CharacterStatistics statistics;
  bool operator==(const CharacterRecord&) const = default;
};

enum class CharacterCreateResult {
  kCreated,
  kNameTaken,
  kAccountHasCharacter,
  kAccountUnavailable
};

class CharacterStore {
 public:
  virtual ~CharacterStore() = default;
  virtual std::optional<CharacterRecord> FindByAccount(
      std::uint64_t account_id) = 0;
  virtual bool NameExists(std::u16string_view name) = 0;
  virtual CharacterCreateResult Create(CharacterRecord& character) = 0;
};
}  // namespace bubble_fighter::server::features::characters

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_STORE_H_
