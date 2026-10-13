#include "protocol/codec/additive_stream_cipher.h"

#include <array>

namespace bubble_fighter::server::protocol {
namespace {
// E006: BFCLIENT02 bytes at 0x015940E8 and 0x01594090.
constexpr std::array<std::uint8_t, 49> kFirstTable = {
    0x5F, 0x3E, 0x32, 0x72, 0xC1, 0x18, 0x46, 0x87, 0x5D, 0x2E,
    0x29, 0x4E, 0x68, 0x79, 0x86, 0x6C, 0xB5, 0x8D, 0x56, 0xF1,
    0x38, 0xC1, 0x48, 0x42, 0x5F, 0x0C, 0xCD, 0x80, 0xE8, 0xE2,
    0xFE, 0xF0, 0x42, 0x5F, 0xF7, 0x59, 0x7E, 0x5F, 0xA5, 0x8D,
    0x3D, 0x76, 0xE7, 0x11, 0x41, 0x62, 0x67, 0x59, 0x7F};
constexpr std::array<std::uint8_t, 24> kSecondTable = {
    0x23, 0x00, 0x24, 0x00, 0x25, 0x00, 0x73, 0x00, 0x65, 0x00, 0x72, 0x00,
    0x74, 0x00, 0x33, 0x00, 0x34, 0x00, 0x25, 0x00, 0x77, 0x00, 0x65, 0x00};
}  // namespace

AdditiveStreamCipher::AdditiveStreamCipher(WireDirection direction)
    : first_index_(direction == WireDirection::kClientToServer ? 0 : 5),
      second_index_(direction == WireDirection::kClientToServer ? 1 : 3) {}

void AdditiveStreamCipher::Encode(std::span<std::uint8_t> bytes) {
  Transform(bytes, true);
}

void AdditiveStreamCipher::Decode(std::span<std::uint8_t> bytes) {
  Transform(bytes, false);
}

void AdditiveStreamCipher::Transform(std::span<std::uint8_t> bytes,
                                     bool encode) {
  for (auto& value : bytes) {
    const int delta = kFirstTable[first_index_] + kSecondTable[second_index_];
    value = static_cast<std::uint8_t>(encode ? value + delta : value - delta);
    first_index_ = (first_index_ + 1) % kFirstTable.size();
    second_index_ = (second_index_ + 1) % kSecondTable.size();
  }
}
}  // namespace bubble_fighter::server::protocol
