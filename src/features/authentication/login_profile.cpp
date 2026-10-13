#include "features/authentication/login_profile.h"

#include <bit>
#include <limits>
#include <stdexcept>
#include <string>

#include "features/characters/character_creation.h"

namespace bubble_fighter::server::features::authentication {
protocol::LoginResponse LoginProfile::Create(
    std::uint64_t account_id, const characters::CharacterRecord& character,
    std::chrono::system_clock::time_point now) {
  if (account_id == 0 || character.account_id != account_id ||
      character.character_id == 0 ||
      character.character_id > std::numeric_limits<std::uint32_t>::max() ||
      characters::CharacterCreation::ValidateName(character.character_name) !=
          0 ||
      !characters::CharacterCreation::SupportsSelection(
          character.character_type, character.color_index)) {
    throw std::runtime_error(
        "Persisted character cannot produce a login profile.");
  }

  // E019: OLE DATE representation. UTC is this server's explicit clock
  // policy; the original production timezone has not been established.
  const auto epoch = std::chrono::sys_days{std::chrono::year{1899} / 12 / 30};
  const double date =
      std::chrono::duration<double, std::ratio<86400>>(now - epoch).count();
  if (date < 25569.0 || date >= 2958466.0) {
    throw std::runtime_error("Server clock outside supported profile range.");
  }

  protocol::LoginResponse response;
  response.result = 0;
  // Local mapping: persistent character id -> 32-bit wire identity.
  // It is checked above, never a truncation of the account primary key.
  response.identity_value = static_cast<std::uint32_t>(character.character_id);
  auto& success = response.success.emplace();
  const auto identifier =
      std::string("LocalAccount.") + std::to_string(account_id);
  success.account_identifier.assign(identifier.begin(), identifier.end());
  success.character_name = character.character_name;
  success.character_type = character.character_type;
  success.server_time_bits = std::bit_cast<std::uint64_t>(date);
  success.game_master_value = 0;

  // +64/+72 keep E039's neutral baseline. E090: +68 is copied to
  // connection +2508 (9A38C4); category 0 skips the lobby's initial C401
  // and its refresh timer (8530C0/858E70). Use the client's free category
  // 60 (10EA190), an explicit single-category local server policy.
  success.unknown_value = 0xFFFFFFFFU;
  success.channel_category = 60;
  success.unknown_connection_value = 0;
  // E039: original LoginNational constructor is zero/empty/false. Local
  // accounts supply no external provider metadata or analytics extension.
  response.national.result = 0;
  response.national.details.emplace();
  response.tail_marker = 0;
  return response;
}
}  // namespace bubble_fighter::server::features::authentication
