#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_ITEM_USE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_ITEM_USE_H_
#include "features/characters/character_store.h"
#include "features/inventory/item_catalog.h"
#include "protocol/inventory/item_use_request.h"
#include "protocol/inventory/item_use_response.h"

namespace bubble_fighter::server::features::inventory {
struct ItemUseResult {
  characters::CharacterRecord character;
  CharacterInventory inventory;
  protocol::ItemUseResponse response;
  std::vector<protocol::TcpPacket> packets;
};

class ItemUse {
 public:
  static bool SupportsRename(const ItemDefinition& item) noexcept;
  // Pure preparation; persistence must check the unique name and commit
  // the name and card decrement together before publishing any packets.
  static ItemUseResult PrepareRename(
      const characters::CharacterRecord& character,
      CharacterInventory inventory, const protocol::ItemUseRequest& request,
      std::uint64_t observed_at_unix_seconds, bool name_taken = false);
};
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_ITEM_USE_H_
