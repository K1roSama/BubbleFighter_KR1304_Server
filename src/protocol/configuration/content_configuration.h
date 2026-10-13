#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CONFIGURATION_CONTENT_CONFIGURATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CONFIGURATION_CONTENT_CONFIGURATION_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace bubble_fighter::server::protocol {
struct ContentConfiguration {
  static constexpr std::size_t kCategory = 14;
  static constexpr std::size_t kMaxEntries = 1024;
  std::u16string create_user_ui_path;
  std::vector<std::uint32_t> character_types;
  std::map<std::uint32_t, std::uint32_t> preview_overrides;
  // Read as binary32 and added to a client date value; units unresolved.
  std::uint32_t time_offset_bits_at52 = 0;
  std::uint32_t item103385_gauge_maximum = 0;
  std::vector<std::uint8_t> Encode() const;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CONFIGURATION_CONTENT_CONFIGURATION_H_
