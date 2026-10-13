#include "network/tcp_server.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <thread>

#include "protocol/codec/additive_stream_cipher.h"
#include "protocol/codec/packet_stream_decoder.h"
#include "protocol/login/connection_greeting.h"
#include "protocol/shop/shop_entry_response.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>

#include <array>
#include <charconv>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <syncstream>

namespace bubble_fighter::server::network {
namespace {
[[noreturn]] void ThrowSocketError(const char* operation) {
  throw std::runtime_error(std::string(operation) + " failed: Winsock " +
                           std::to_string(WSAGetLastError()));
}

class WinsockSession {
 public:
  WinsockSession() {
    WSADATA data{};
    const int error = WSAStartup(MAKEWORD(2, 2), &data);
    if (error != 0) {
      throw std::runtime_error("WSAStartup failed: " + std::to_string(error));
    }
  }
  ~WinsockSession() { WSACleanup(); }
  WinsockSession(const WinsockSession&) = delete;
  WinsockSession& operator=(const WinsockSession&) = delete;
};

class SocketHandle {
 public:
  explicit SocketHandle(SOCKET value) : value_(value) {
    if (value == INVALID_SOCKET) {
      ThrowSocketError("socket/accept");
    }
  }
  ~SocketHandle() { closesocket(value_); }
  SocketHandle(const SocketHandle&) = delete;
  SocketHandle& operator=(const SocketHandle&) = delete;
  SOCKET Get() const { return value_; }

 private:
  SOCKET value_;
};

void SendAll(SOCKET socket, std::span<const std::uint8_t> bytes) {
  std::size_t sent = 0;
  while (sent < bytes.size()) {
    const auto result =
        send(socket, reinterpret_cast<const char*>(bytes.data() + sent),
             static_cast<int>(bytes.size() - sent), 0);
    if (result == SOCKET_ERROR) {
      ThrowSocketError("send");
    }
    if (result == 0) {
      throw std::runtime_error("Connection closed while sending data.");
    }
    sent += static_cast<std::size_t>(result);
  }
}

void ReceiveConnection(SOCKET socket, ConnectionHandlers handlers,
                       std::uint32_t idle_timeout_seconds,
                       std::stop_token stop = {}) {
  if (!handlers.on_packet) {
    throw std::invalid_argument("Missing packet handler.");
  }
  // Local resource policy; original server timeout is not established.
  const DWORD timeout_milliseconds = idle_timeout_seconds * 1000;
  for (const auto option : {SO_RCVTIMEO, SO_SNDTIMEO}) {
    if (setsockopt(socket, SOL_SOCKET, option,
                   reinterpret_cast<const char*>(&timeout_milliseconds),
                   sizeof(timeout_milliseconds)) == SOCKET_ERROR) {
      ThrowSocketError("setsockopt timeout");
    }
  }
  SendAll(socket, protocol::ConnectionGreeting::Create());
  protocol::AdditiveStreamCipher cipher(
      protocol::WireDirection::kClientToServer);
  protocol::AdditiveStreamCipher outgoing_cipher(
      protocol::WireDirection::kServerToClient);
  protocol::PacketStreamDecoder decoder;
  std::array<std::uint8_t, 4096> buffer{};
  std::size_t packet_count = 0;
  const auto deliver = [&](const PacketHandlingResult& outcome) {
    for (const auto& response : outcome.responses) {
      auto outgoing = protocol::TcpPacketCodec::Encode(
          response,
          response.message_id == protocol::ShopEntryResponse::kMessageId
              ? static_cast<std::uint32_t>(
                    protocol::ShopEntryResponse::kMaxPayloadBytes)
              : protocol::TcpPacketCodec::kDefaultMaxPayloadSize);
      outgoing_cipher.Encode(outgoing);
      SendAll(socket, outgoing);
      std::osyncstream(std::cout)
          << "ServerToClient MessageId=" << response.message_id
          << " PayloadBytes=" << response.payload.size() << std::endl;
    }
    if (outcome.close_connection) {
      shutdown(socket, SD_SEND);
      std::osyncstream(std::cout)
          << "Connection closed by message handler. Packets=" << packet_count
          << std::endl;
    }
    return outcome.close_connection;
  };
  auto last_receive = std::chrono::steady_clock::now();
  while (!stop.stop_requested()) {
    // Serialized tick and packet callbacks share this connection's
    // cipher stream. No sleeps in business handlers or timer thread.
    if (handlers.on_tick && deliver(handlers.on_tick())) {
      return;
    }
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(socket, &readable);
    timeval interval{0, 100000};
    const int available = select(0, &readable, nullptr, nullptr, &interval);
    if (available == SOCKET_ERROR) {
      ThrowSocketError("select");
    }
    if (available == 0) {
      if (std::chrono::steady_clock::now() - last_receive >=
          std::chrono::seconds{idle_timeout_seconds}) {
        throw std::runtime_error("Connection receive idle timeout.");
      }
      continue;
    }
    const int received = recv(socket, reinterpret_cast<char*>(buffer.data()),
                              static_cast<int>(buffer.size()), 0);
    if (received == SOCKET_ERROR) {
      ThrowSocketError("recv");
    }
    if (received == 0) {
      decoder.Finish();
      std::osyncstream(std::cout)
          << "Connection closed. Packets=" << packet_count << std::endl;
      return;
    }
    last_receive = std::chrono::steady_clock::now();
    auto bytes = std::span(buffer).first(static_cast<std::size_t>(received));
    cipher.Decode(bytes);
    for (const auto& packet : decoder.Feed(bytes)) {
      ++packet_count;
      // Payloads can contain credentials. Log metadata only.
      std::osyncstream(std::cout)
          << "ClientToServer MessageId=" << packet.message_id
          << " PayloadBytes=" << packet.payload.size() << std::endl;
      if (deliver(handlers.on_packet(packet))) {
        return;
      }
    }
  }
}
}  // namespace

std::uint32_t TcpServer::ParseListenAddress(std::string_view address) {
  std::uint32_t result = 0;
  for (int part = 0; part < 4; ++part) {
    const auto dot = address.find('.');
    const auto digits = address.substr(0, dot);
    if (digits.empty() || digits.size() > 3 ||
        (digits.size() > 1 && digits.front() == '0') ||
        (part == 3) != (dot == std::string_view::npos)) {
      throw std::invalid_argument(
          "Listen address must be dotted-decimal IPv4.");
    }
    unsigned int value = 0;
    const auto parsed =
        std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (parsed.ec != std::errc{} ||
        parsed.ptr != digits.data() + digits.size() || value > 255) {
      throw std::invalid_argument(
          "Listen address must be dotted-decimal IPv4.");
    }
    result = (result << 8) | value;
    if (part != 3) {
      address.remove_prefix(dot + 1);
    }
  }
  return result;
}

int TcpServer::Run(std::uint16_t port, bool once,
                   const PacketHandlerFactory& create_handler,
                   std::uint32_t idle_timeout_seconds,
                   const std::function<void(std::uint16_t)>& on_listening,
                   std::stop_token stop,
                   std::string_view listen_address) const {
  if (idle_timeout_seconds == 0 || idle_timeout_seconds > 600) {
    throw std::invalid_argument("Invalid idle timeout.");
  }
  const auto ipv4 = ParseListenAddress(listen_address);
  const WinsockSession winsock;
  const SocketHandle listener(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
  const BOOL exclusive_address = TRUE;
  if (setsockopt(listener.Get(), SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                 reinterpret_cast<const char*>(&exclusive_address),
                 sizeof(exclusive_address)) == SOCKET_ERROR) {
    ThrowSocketError("setsockopt exclusive address");
  }
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(ipv4);
  address.sin_port = htons(port);
  if (bind(listener.Get(), reinterpret_cast<const sockaddr*>(&address),
           sizeof(address)) == SOCKET_ERROR) {
    ThrowSocketError("bind");
  }
  if (listen(listener.Get(), SOMAXCONN) == SOCKET_ERROR) {
    ThrowSocketError("listen");
  }
  int address_length = sizeof(address);
  if (getsockname(listener.Get(), reinterpret_cast<sockaddr*>(&address),
                  &address_length) == SOCKET_ERROR) {
    ThrowSocketError("getsockname");
  }
  std::osyncstream(std::cout) << "Listening on " << listen_address << ':'
                              << ntohs(address.sin_port) << std::endl;
  if (on_listening) {
    on_listening(ntohs(address.sin_port));
  }
  struct Worker {
    std::shared_ptr<SocketHandle> socket;
    std::shared_ptr<std::atomic_bool> finished;
    std::jthread thread;
  };
  std::vector<Worker> workers;
  workers.reserve(64);
  // Joins are bounded by shutdown, even if a peer is not reading replies.
  const auto stop_workers = [&] {
    for (auto& worker : workers) {
      worker.thread.request_stop();
      shutdown(worker.socket->Get(), SD_BOTH);
    }
    workers.clear();
  };
  try {
    while (!stop.stop_requested()) {
      std::erase_if(workers, [](const Worker& worker) {
        return worker.finished->load();
      });
      fd_set readable;
      FD_ZERO(&readable);
      FD_SET(listener.Get(), &readable);
      timeval interval{0, 100000};
      const auto available = select(0, &readable, nullptr, nullptr, &interval);
      if (available == SOCKET_ERROR) {
        ThrowSocketError("listener select");
      }
      if (available == 0) {
        continue;
      }
      auto connection = std::make_shared<SocketHandle>(
          accept(listener.Get(), nullptr, nullptr));
      if (!once && workers.size() >= 64) {
        continue;
      }
      try {
        auto handlers = create_handler();
        if (once) {
          ReceiveConnection(connection->Get(), std::move(handlers),
                            idle_timeout_seconds, stop);
          return 0;
        }
        auto finished = std::make_shared<std::atomic_bool>(false);
        std::jthread thread(
            [connection, finished, handlers = std::move(handlers),
             idle_timeout_seconds](std::stop_token connection_stop) mutable {
              try {
                ReceiveConnection(connection->Get(), std::move(handlers),
                                  idle_timeout_seconds, connection_stop);
              } catch (const std::exception& error) {
                if (!connection_stop.stop_requested()) {
                  std::osyncstream(std::cerr)
                      << "Connection rejected: " << error.what() << std::endl;
                }
              }
              // Destroy session/room leases before announcing completion.
              handlers = {};
              finished->store(true);
            });
        workers.push_back(
            {std::move(connection), std::move(finished), std::move(thread)});
      } catch (const std::exception& error) {
        std::osyncstream(std::cerr)
            << "Connection rejected: " << error.what() << std::endl;
        if (once) {
          return 1;
        }
      }
    }
  } catch (...) {
    stop_workers();
    throw;
  }
  stop_workers();
  return 0;
}
}  // namespace bubble_fighter::server::network
