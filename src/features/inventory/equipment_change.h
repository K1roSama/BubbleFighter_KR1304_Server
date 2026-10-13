#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_EQUIPMENT_CHANGE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_EQUIPMENT_CHANGE_H_

#include "features/inventory/equipment_view.h"

namespace bubble_fighter::server::features::inventory {
struct EquipmentChangeResult {
  CharacterInventory inventory;
  std::vector<protocol::TcpPacket> packets;
};

class EquipmentChange {
 public:
  // Pure preparation: validates owned ordinary clothing/weapons, removes
  // conflicting positions and encodes the entire update before any write.
  // Character selection, ArticleSlot and special functions remain separate.
  static EquipmentChangeResult Prepare(
      const characters::CharacterRecord& character,
      CharacterInventory inventory, std::uint64_t item_identifier, bool equip,
      std::uint64_t observed_at_unix_seconds);
};
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_EQUIPMENT_CHANGE_H_
