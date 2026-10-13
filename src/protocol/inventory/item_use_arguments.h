#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_ITEM_USE_ARGUMENTS_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_ITEM_USE_ARGUMENTS_H_
#include <string>
#include <vector>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
// E067: uint32 count followed by uint32-length UTF-16LE strings.
// These aggregate limits are local resource policies, not original limits.
struct ItemUseArguments {
  static constexpr std::size_t kMaxCount = 32;
  static constexpr std::size_t kMaxPayloadBytes = 65536;
  static void Write(PacketWriter& writer,
                    const std::vector<std::u16string>& values);
  static std::vector<std::u16string> Read(PacketReader& reader);
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_ITEM_USE_ARGUMENTS_H_
