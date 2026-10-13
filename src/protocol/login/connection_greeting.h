#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CONNECTION_GREETING_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CONNECTION_GREETING_H_

#include <array>
#include <cstddef>
#include <cstdint>

namespace bubble_fighter::server::protocol {
class ConnectionGreeting {
 public:
  static constexpr std::size_t kSize = 258;
  static std::array<std::uint8_t, kSize> Create();
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_CONNECTION_GREETING_H_
