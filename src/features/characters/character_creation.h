#ifndef BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_CREATION_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_CREATION_H_

#include "features/characters/character_store.h"
#include "protocol/characters/create_user_info_request.h"

namespace bubble_fighter::server::features::characters {
class CharacterCreation {
 public:
  // Explicit local name policy, not reconstructed production rules.
  static constexpr std::size_t kMaxNameCodeUnits = 16;
  static std::uint32_t ValidateName(std::u16string_view name);
  static bool SupportsSelection(std::uint32_t character_type,
                                std::uint32_t color_index);
  explicit CharacterCreation(CharacterStore& store);
  std::uint32_t CheckName(std::u16string_view name);
  std::uint32_t Create(std::uint64_t account_id,
                       const protocol::CreateUserInfoRequest& request);

 private:
  CharacterStore& store_;
};
}  // namespace bubble_fighter::server::features::characters

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_CHARACTERS_CHARACTER_CREATION_H_
