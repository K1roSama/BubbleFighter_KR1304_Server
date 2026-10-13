#ifndef BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_LOGIN_PROFILE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_LOGIN_PROFILE_H_

#include <chrono>

#include "features/characters/character_store.h"
#include "protocol/login/login_response.h"

namespace bubble_fighter::server::features::authentication {
class LoginProfile {
 public:
  static protocol::LoginResponse Create(
      std::uint64_t trusted_account_id,
      const characters::CharacterRecord& character,
      std::chrono::system_clock::time_point now);
};
}  // namespace bubble_fighter::server::features::authentication

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_AUTHENTICATION_LOGIN_PROFILE_H_
