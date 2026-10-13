#include "protocol/codec/packet_reader.h"

#include <stdexcept>

#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
PacketReader::PacketReader(std::span<const std::uint8_t> bytes)
    : remaining_(bytes) {}

std::span<const std::uint8_t> PacketReader::ReadBytes(std::size_t size) {
  if (size > remaining_.size()) {
    throw std::runtime_error("Truncated message field.");
  }
  const auto bytes = remaining_.first(size);
  remaining_ = remaining_.subspan(size);
  return bytes;
}

std::uint8_t PacketReader::ReadUInt8() { return ReadBytes(1).front(); }

std::uint16_t PacketReader::ReadUInt16() {
  const auto bytes = ReadBytes(2);
  return static_cast<std::uint16_t>(
      bytes[0] | (static_cast<std::uint16_t>(bytes[1]) << 8));
}

std::size_t PacketReader::RemainingBytes() const noexcept {
  return remaining_.size();
}

std::uint32_t PacketReader::ReadUInt32() {
  const auto bytes = ReadBytes(4);
  std::uint32_t value = 0;
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    value |= static_cast<std::uint32_t>(bytes[index]) << (8 * index);
  }
  return value;
}

std::uint64_t PacketReader::ReadUInt64() {
  const auto bytes = ReadBytes(8);
  std::uint64_t value = 0;
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    value |= static_cast<std::uint64_t>(bytes[index]) << (8 * index);
  }
  return value;
}

std::u16string PacketReader::ReadUtf16() {
  const auto count = ReadUInt32();
  if (count > PacketWriter::kMaxStringCodeUnits) {
    throw std::length_error("String exceeds supported code-unit limit.");
  }
  const auto bytes = ReadBytes(static_cast<std::size_t>(count) * 2);
  std::u16string value;
  value.reserve(count);
  for (std::size_t offset = 0; offset < bytes.size(); offset += 2) {
    value.push_back(static_cast<char16_t>(
        static_cast<std::uint16_t>(bytes[offset]) |
        (static_cast<std::uint16_t>(bytes[offset + 1]) << 8)));
  }
  return value;
}

void PacketReader::RequireEnd() const {
  if (!remaining_.empty()) {
    throw std::runtime_error("Unexpected trailing message fields.");
  }
}
}  // namespace bubble_fighter::server::protocol
