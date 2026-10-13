#include "protocol/codec/packet_writer.h"

#include <stdexcept>
#include <utility>

namespace bubble_fighter::server::protocol {
void PacketWriter::WriteUInt8(std::uint8_t value) { bytes_.push_back(value); }

void PacketWriter::WriteUInt16(std::uint16_t value) {
  bytes_.push_back(static_cast<std::uint8_t>(value));
  bytes_.push_back(static_cast<std::uint8_t>(value >> 8));
}

void PacketWriter::WriteUInt32(std::uint32_t value) {
  for (std::size_t index = 0; index < 4; ++index) {
    bytes_.push_back(static_cast<std::uint8_t>(value >> (index * 8)));
  }
}

void PacketWriter::WriteUInt64(std::uint64_t value) {
  for (std::size_t index = 0; index < 8; ++index) {
    bytes_.push_back(static_cast<std::uint8_t>(value >> (index * 8)));
  }
}

void PacketWriter::WriteUtf16(std::u16string_view value) {
  if (value.size() > kMaxStringCodeUnits) {
    throw std::length_error("String exceeds supported code-unit limit.");
  }
  WriteUInt32(static_cast<std::uint32_t>(value.size()));
  for (const auto unit : value) {
    bytes_.push_back(static_cast<std::uint8_t>(unit));
    bytes_.push_back(static_cast<std::uint8_t>(unit >> 8));
  }
}

std::vector<std::uint8_t> PacketWriter::TakeBytes() {
  auto result = std::move(bytes_);
  bytes_.clear();
  return result;
}
}  // namespace bubble_fighter::server::protocol
