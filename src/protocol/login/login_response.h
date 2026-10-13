#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_LOGIN_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_LOGIN_RESPONSE_H_

#include <optional>
#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
struct LoginSuccessFields {
  // E019: copied to crash-report ACCOUNT_ID and CHAR_NAME respectively.
  std::u16string account_identifier;
  std::u16string character_name;
  // E023: resource selection compares this with ForDao, ForDizini, etc.
  std::uint32_t character_type = 0;
  std::uint32_t unknown_value = 0;
  std::uint32_t channel_category = 0;
  std::uint32_t unknown_connection_value = 0;
  // E019: OLE DATE double, preserved as raw little-endian IEEE-754 bits.
  // The original server's time zone has not been established.
  std::uint64_t server_time_bits = 0;
  // E023: signed-positive values enable the client's GM branches.
  // Preserve the wire value; privilege levels have not been established.
  std::uint32_t game_master_value = 0;

  bool operator==(const LoginSuccessFields&) const = default;
};

struct LoginNationalDetails {
  std::uint8_t byte_value = 0;
  std::u16string first_string;
  std::u16string second_string;
  // The original reader converts nonzero to true. Preserve the wire value.
  std::uint32_t boolean_value = 0;

  bool operator==(const LoginNationalDetails&) const = default;
};

struct LoginNationalFields {
  std::uint32_t result = 0;
  std::optional<LoginNationalDetails> details;

  bool operator==(const LoginNationalFields&) const = default;
};

// BFCLIENT02: 0x0099C370 reads base response, national data, then a marker.
// Unknown field meanings remain neutral; encoding does not approve a login.
struct LoginResponse {
  static constexpr std::uint16_t kMessageId = 0x0003;
  static constexpr std::uint8_t kExtensionMarker = 0x3A;

  std::uint32_t result = 0;
  // Client object +32: included in both result branches; meaning depends on
  // result.
  std::uint32_t identity_value = 0;
  std::optional<LoginSuccessFields> success;
  LoginNationalFields national;
  // Explicit marker required by this codec, even when it is not 0x3A.
  std::uint8_t tail_marker = 0;
  std::optional<std::uint32_t> tail_value;

  TcpPacket Encode() const;
  static LoginResponse Decode(const TcpPacket& packet);
  bool operator==(const LoginResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_LOGIN_LOGIN_RESPONSE_H_
