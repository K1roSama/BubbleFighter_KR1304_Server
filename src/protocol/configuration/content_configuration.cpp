#include "protocol/configuration/content_configuration.h"

#include <stdexcept>

#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
std::vector<std::uint8_t> ContentConfiguration::Encode() const {
  if (character_types.size() > kMaxEntries ||
      preview_overrides.size() > kMaxEntries) {
    throw std::invalid_argument(
        "Content configuration exceeds local entry limit.");
  }
  PacketWriter writer;
  writer.WriteUtf16(create_user_ui_path);
  writer.WriteUInt32(static_cast<std::uint32_t>(character_types.size()));
  for (const auto type : character_types) {
    writer.WriteUInt32(type);
  }
  writer.WriteUInt32(static_cast<std::uint32_t>(preview_overrides.size()));
  for (const auto& [type, value] : preview_overrides) {
    writer.WriteUInt32(type);
    writer.WriteUInt32(value);
  }
  writer.WriteUInt32(time_offset_bits_at52);
  writer.WriteUInt32(item103385_gauge_maximum);
  return writer.TakeBytes();
}
}  // namespace bubble_fighter::server::protocol
