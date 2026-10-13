#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_LOGIN_REQUEST_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_LOGIN_REQUEST_H_

#include <array>
#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// BFCLIENT02: LoginStage -> 0x0085FF00 -> 0x010C6A00 / 0x010C74A0.
// This is a wire record. Decoding it does not authenticate its Passport.
struct LoginRequest {
  static constexpr std::uint16_t kMessageId = 0x0003;
  // Local support limit based on the ten-adapter loop at 0x010BBCB0.
  // The vector serializer itself does not impose this limit.
  static constexpr std::uint32_t kMaxLocalIpv4Addresses = 10;

  std::u16string nexon_passport;
  std::u16string hardware_id;
  std::u16string unknown_string;
  // Octets in textual address order, e.g. {192, 0, 2, 1}.
  // The client serializes inet_addr's four memory bytes unchanged.
  std::vector<std::array<std::uint8_t, 4>> local_ipv4_addresses;
  std::uint32_t resource_value = 0;
  std::uint32_t unknown_value = 0;
  std::u16string platform_string;

  TcpPacket Encode() const;
  static LoginRequest Decode(const TcpPacket& packet);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_LOGIN_REQUEST_H_
