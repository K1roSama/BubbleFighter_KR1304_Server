#ifndef BUBBLEFIGHTER_SERVER_NETWORK_CONNECTION_HEARTBEAT_H_
#define BUBBLEFIGHTER_SERVER_NETWORK_CONNECTION_HEARTBEAT_H_

#include <chrono>

#include "network/tcp_server.h"

namespace bubble_fighter::server::network {
class ConnectionHeartbeat {
 public:
  using Clock = std::chrono::steady_clock;
  using Now = std::function<Clock::time_point()>;

  // Serialized by the TCP connection loop. Local interval/response limit:
  // min(10 seconds, receive-idle timeout / 3). No original timing claim.
  static ConnectionHandlers Wrap(ConnectionHandlers handlers,
                                 std::function<bool()> authenticated,
                                 std::uint32_t idle_timeout_seconds = 30,
                                 Now now = Clock::now);
};
}  // namespace bubble_fighter::server::network

#endif  // BUBBLEFIGHTER_SERVER_NETWORK_CONNECTION_HEARTBEAT_H_
