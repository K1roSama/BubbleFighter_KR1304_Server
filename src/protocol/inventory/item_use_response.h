#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_ITEM_USE_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_ITEM_USE_RESPONSE_H_
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/inventory/item_use_arguments.h"

namespace bubble_fighter::server::protocol {
struct ItemUseResponse {
  static constexpr std::uint16_t kMessageId = 813;
  // Result meanings are function-specific. These are Function=3 only.
  static constexpr std::uint32_t kRenameSuccess = 0, kRenameNameTaken = 10,
                                 kRenameBannedName = 11,
                                 kRenameInvalidLength = 12,
                                 kRenameInvalidCharacters = 13;
  std::uint32_t result = 0;
  std::uint32_t item_id = 0;
  std::vector<std::u16string> arguments;
  TcpPacket Encode() const;
  static ItemUseResponse Decode(const TcpPacket& packet);
  bool operator==(const ItemUseResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_ITEM_USE_RESPONSE_H_
