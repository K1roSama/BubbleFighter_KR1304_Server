#include "protocol/login/connection_greeting.h"

namespace bubble_fighter::server::protocol {
std::array<std::uint8_t, ConnectionGreeting::kSize>
ConnectionGreeting::Create() {
  // E007: sub_10BA7D0 consumes 258 plaintext bytes; sub_99EAD0
  // accepts a little-endian word equal to 4. Remaining bytes are unused
  // on that branch, so this server fills them with zero.
  std::array<std::uint8_t, kSize> greeting{};
  greeting[0] = 4;
  return greeting;
}
}  // namespace bubble_fighter::server::protocol
