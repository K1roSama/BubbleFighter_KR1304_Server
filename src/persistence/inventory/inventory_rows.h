#ifndef BUBBLEFIGHTER_SERVER_PERSISTENCE_INVENTORY_INVENTORY_ROWS_H_
#define BUBBLEFIGHTER_SERVER_PERSISTENCE_INVENTORY_INVENTORY_ROWS_H_

#include <mysql.h>

#include "features/characters/character_store.h"
#include "features/inventory/cash_inventory.h"
#include "features/inventory/inventory_slot.h"
#include "features/inventory/weapon_preset_state.h"

namespace bubble_fighter::server::persistence::inventory_sql {
using features::inventory::CashInventory;
using features::inventory::CharacterInventory;
using features::inventory::InventorySlot;
using features::inventory::WeaponPresetState;
bool LockOwner(MYSQL* connection, std::uint64_t account_id,
               std::uint64_t character_id);
CharacterInventory LoadSlots(MYSQL* connection, std::uint64_t account_id,
                             std::uint64_t character_id);
WeaponPresetState LoadPresets(MYSQL* connection,
                              const CharacterInventory& inventory);
void PersistPresets(MYSQL* connection, std::uint64_t character_id,
                    const WeaponPresetState& state);
void PersistEquipment(MYSQL* connection, const CharacterInventory& before,
                      CharacterInventory& after);
void PersistGrant(MYSQL* connection,
                  const features::characters::CharacterRecord& character,
                  InventorySlot& slot);
CashInventory LoadCashSlots(MYSQL* connection, std::uint64_t account_id,
                            std::uint64_t character_id);
features::characters::CharacterRecord ReadCurrentCharacter(
    MYSQL* connection, const features::characters::CharacterRecord& character);
}  // namespace bubble_fighter::server::persistence::inventory_sql

#endif  // BUBBLEFIGHTER_SERVER_PERSISTENCE_INVENTORY_INVENTORY_ROWS_H_
