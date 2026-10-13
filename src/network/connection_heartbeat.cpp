#include "network/connection_heartbeat.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

#include "protocol/login/client_ping_response.h"
#include "protocol/login/server_ping_request.h"

namespace bubble_fighter::server::network {
namespace {
struct HeartbeatState {
  using Clock = ConnectionHeartbeat::Clock;
  ConnectionHandlers handlers;
  std::function<bool()> authenticated;
  ConnectionHeartbeat::Now read_clock;
  std::chrono::milliseconds interval;
  std::optional<Clock::time_point> last_observed;
  std::optional<Clock::time_point> last_activity;
  std::optional<Clock::time_point> pending_since;
  bool closed = false;

  Clock::time_point Observe() {
    const auto observed_at = read_clock();
    if (last_observed && observed_at < *last_observed) {
      throw std::runtime_error("Heartbeat clock moved backwards.");
    }
    last_observed = observed_at;
    return observed_at;
  }

  bool Expired(Clock::time_point now) {
    if ((last_activity && !authenticated()) ||
        (pending_since && now - *pending_since >= interval)) {
      closed = true;
    }
    return closed;
  }

  void Activate(Clock::time_point now) {
    if (!last_activity && authenticated()) {
      last_activity = now;
    }
  }

  PacketHandlingResult Process(const protocol::TcpPacket& packet) {
    if (closed) {
      return {{}, true};
    }
    const auto now = Observe();
    if (Expired(now)) {
      return {{}, true};
    }
    if (packet.message_id == protocol::ClientPingResponse::kMessageId) {
      protocol::ClientPingResponse::Decode(packet);
      if (!last_activity || !pending_since) {
        throw std::invalid_argument("Unsolicited client ping response.");
      }
      last_activity = now;
      pending_since.reset();
      return {};
    }
    auto result = handlers.on_packet(packet);
    if (result.close_connection) {
      closed = true;
      return result;
    }
    const auto after = Observe();
    if (Expired(after)) {
      return {{}, true};
    }
    Activate(after);
    return result;
  }

  PacketHandlingResult Poll() {
    if (closed) {
      return {{}, true};
    }
    if (Expired(Observe())) {
      return {{}, true};
    }
    auto result =
        handlers.on_tick ? handlers.on_tick() : PacketHandlingResult{};
    if (result.close_connection) {
      closed = true;
      return result;
    }
    const auto now = Observe();
    if (Expired(now)) {
      return {{}, true};
    }
    Activate(now);
    if (last_activity && !pending_since && now - *last_activity >= interval) {
      result.responses.push_back(protocol::ServerPingRequest{}.Encode());
      pending_since = now;
    }
    return result;
  }
};
}  // namespace

ConnectionHandlers ConnectionHeartbeat::Wrap(
    ConnectionHandlers handlers, std::function<bool()> authenticated,
    std::uint32_t idle_timeout_seconds, Now now) {
  if (!handlers.on_packet || !authenticated || !now ||
      idle_timeout_seconds == 0 || idle_timeout_seconds > 600) {
    throw std::invalid_argument("Invalid heartbeat configuration.");
  }
  const auto interval = std::chrono::milliseconds{
      std::min<std::uint32_t>(10000, idle_timeout_seconds * 1000 / 3)};
  auto state = std::make_shared<HeartbeatState>();
  state->handlers = std::move(handlers);
  state->authenticated = std::move(authenticated);
  state->read_clock = std::move(now);
  state->interval = interval;
  ConnectionHandlers result;
  result.on_packet = [state](const protocol::TcpPacket& packet) {
    try {
      return state->Process(packet);
    } catch (...) {
      state->closed = true;
      throw;
    }
  };
  result.on_tick = [state] {
    try {
      return state->Poll();
    } catch (...) {
      state->closed = true;
      throw;
    }
  };
  return result;
}
}  // namespace bubble_fighter::server::network
