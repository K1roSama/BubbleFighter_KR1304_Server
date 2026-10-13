#include "persistence/my_sql_inventory_store.h"

#include <cstdint>
#include <filesystem>
#include <optional>

#include "features/characters/character_store.h"
#include "features/inventory/default_inventory.h"
#include "persistence/inventory/inventory_rows.h"
#include "persistence/my_sql_commands.h"

namespace bubble_fighter::server::persistence {
using namespace features::inventory;
using namespace my_sql_commands;
using namespace inventory_sql;

MySqlInventoryStore::MySqlInventoryStore(
    const std::filesystem::path& configuration)
    : connection_(configuration) {}

std::optional<CharacterInventory> MySqlInventoryStore::Load(
    std::uint64_t authenticated_account_id, std::uint64_t character_id,
    std::uint64_t observed_at_unix_seconds) {
  auto* connection = connection_.Handle();
  Transaction transaction(connection);
  if (!LockOwner(connection, authenticated_account_id, character_id)) {
    return {};
  }
  auto inventory =
      LoadSlots(connection, authenticated_account_id, character_id);
  (void)inventory.ToRecords(authenticated_account_id, observed_at_unix_seconds);
  transaction.Commit();
  return inventory;
}

std::optional<InventorySnapshot> MySqlInventoryStore::LoadSnapshot(
    std::uint64_t authenticated_account_id, std::uint64_t character_id,
    std::uint64_t observed_at_unix_seconds) {
  auto* connection = connection_.Handle();
  Transaction transaction(connection);
  if (!LockOwner(connection, authenticated_account_id, character_id)) {
    return {};
  }
  InventorySnapshot snapshot{
      LoadSlots(connection, authenticated_account_id, character_id),
      LoadCashSlots(connection, authenticated_account_id, character_id)};
  (void)snapshot.inventory.ToRecords(authenticated_account_id,
                                     observed_at_unix_seconds);
  snapshot.presets = LoadPresets(connection, snapshot.inventory);
  transaction.Commit();
  return snapshot;
}

std::optional<InventorySnapshot> MySqlInventoryStore::LoadMenuSnapshot(
    const features::characters::CharacterRecord& character,
    std::uint64_t observed_at_unix_seconds) {
  auto* connection = connection_.Handle();
  Transaction transaction(connection);
  if (!LockOwner(connection, character.account_id, character.character_id)) {
    return {};
  }
  const auto current = ReadCurrentCharacter(connection, character);
  InventorySnapshot snapshot{
      DefaultInventory::Prepare(
          current,
          LoadSlots(connection, current.account_id, current.character_id),
          observed_at_unix_seconds),
      LoadCashSlots(connection, current.account_id, current.character_id)};
  snapshot.presets = LoadPresets(connection, snapshot.inventory);
  (void)snapshot.presets
      .CreateNotification(current, snapshot.inventory, observed_at_unix_seconds)
      .Encode();
  bool inserted = false;
  for (auto& slot : snapshot.inventory.slots) {
    if (slot.revision != 0) {
      continue;
    }
    PersistGrant(connection, current, slot);
    inserted = true;
  }
  if (inserted) {
    PersistPresets(connection, current.character_id, snapshot.presets);
  }
  transaction.Commit();
  return snapshot;
}

}  // namespace bubble_fighter::server::persistence
