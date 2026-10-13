#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_PRESENTATION_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_PRESENTATION_H_

#include "features/inventory/equipment_view.h"

namespace bubble_fighter::server::features::inventory {

// Preserve the former menu-snapshot identity checks and equipment validation
// for commerce callers, without allocating unrelated menu or scene state.
protocol::PartySupplementaryRecord CreateInventoryEquipment(
    const characters::CharacterRecord& character,
    const CharacterInventory& inventory,
    std::uint64_t observed_at_unix_seconds);

}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_PRESENTATION_H_
