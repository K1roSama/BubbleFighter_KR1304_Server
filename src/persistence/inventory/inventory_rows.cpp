#include "persistence/inventory/inventory_rows.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

#include "features/characters/character_store.h"
#include "features/inventory/cash_inventory.h"
#include "features/inventory/inventory_slot.h"
#include "persistence/my_sql_commands.h"
#include "protocol/inventory/cash_inventory_update_notification.h"
#include "protocol/inventory/inventory_item_record.h"

namespace bubble_fighter::server::persistence::inventory_sql {
using namespace features::inventory;
using namespace my_sql_commands;

bool LockOwner(MYSQL* connection, std::uint64_t account_id,
               std::uint64_t character_id) {
  if (account_id == 0 || character_id == 0 || character_id > 0xFFFFFFFFULL) {
    throw std::invalid_argument("Invalid inventory owner.");
  }
  // Every inventory writer takes the character lock before inspecting
  // rows, so concurrent additions cannot bypass the aggregate limits.
  Statement query(
      connection,
      "SELECT c.CharacterId FROM Characters c JOIN Accounts a ON a.AccountId = c.AccountId "
      "WHERE c.CharacterId = ? AND c.AccountId = ? AND a.IsDisabled = 0 FOR UPDATE");
  std::array parameters{Number(character_id), Number(account_id)};
  query.Execute(parameters);
  std::uint64_t id = 0;
  std::array results{Number(id)};
  return query.Fetch(results);
}

CharacterInventory LoadSlots(MYSQL* connection, std::uint64_t account_id,
                             std::uint64_t character_id) {
  CharacterInventory inventory{account_id, character_id, {}};
  // One over the supported count detects an oversized database without
  // allocating an unbounded result. Details are versioned opaque wire
  // fields until their business semantics are established.
  Statement query(
      connection,
      "SELECT ItemId, UsageKind, Quantity, ExpiresAtUnixSeconds, IsEquipped, "
      "DetailVersion, DetailData, Revision FROM InventorySlots WHERE CharacterId = ? ORDER BY ItemId LIMIT 1025");
  std::array parameters{Number(character_id)};
  query.Execute(parameters);
  InventorySlot slot;
  std::uint8_t usage = 0, equipped = 0, version = 0;
  std::uint64_t expiry = 0;
  bool expiry_null = false;
  std::vector<std::uint8_t> detail(
      protocol::InventoryItemRecord::kMaxUncompressedBytes);
  unsigned long length = 0;
  std::array results{
      Number(slot.item_id),
      Number(usage),
      Number(slot.quantity),
      Number(expiry),
      Number(equipped),
      Number(version),
      Bytes(detail.data(), static_cast<unsigned long>(detail.size())),
      Number(slot.revision)};
  results[3].is_null = &expiry_null;
  results[6].length = &length;
  std::size_t total_bytes = 4;
  std::size_t total_entries = 0;
  while (query.Fetch(results)) {
    if (inventory.slots.size() ==
            protocol::InventoryItemRecord::kMaxCollectionEntries ||
        version != 1 || equipped > 1 || slot.revision == 0 || length < 29 ||
        length > detail.size() ||
        length - 4 > protocol::InventoryItemRecord::kMaxUncompressedBytes -
                         total_bytes) {
      throw std::runtime_error("Unsupported stored inventory slot.");
    }
    total_bytes += length - 4;
    const auto records = protocol::InventoryItemRecord::DecodeList(
        std::span(detail).first(length));
    if (records.size() != 1 || records[0].key != 0 ||
        records[0].value_at4 != 0 || records[0].bits_at8 != 0 ||
        records[0].flags != 0) {
      throw std::runtime_error("Invalid stored inventory detail envelope.");
    }
    std::size_t entries = 1 + records[0].attributes_at24.size();
    for (const auto& attribute : records[0].attributes_at24) {
      entries += attribute.values_at8.size();
    }
    if (entries > protocol::InventoryItemRecord::kMaxTotalCollectionEntries -
                      total_entries) {
      throw std::runtime_error(
          "Stored inventory exceeds the aggregate entry limit.");
    }
    total_entries += entries;
    slot.usage = static_cast<InventoryUsage>(usage);
    slot.equipped = equipped != 0;
    slot.expires_at_unix_seconds =
        expiry_null ? std::nullopt : std::optional(expiry);
    slot.text_at36 = records[0].text_at36;
    slot.attributes_at24 = records[0].attributes_at24;
    inventory.slots.push_back(slot);
  }
  return inventory;
}
WeaponPresetState LoadPresets(MYSQL* connection,
                              const CharacterInventory& inventory) {
  WeaponPresetState state;
  std::array parameters{Number(inventory.character_id)};
  std::uint8_t count = 0;
  bool present = false;
  {
    // Statement uses an unbuffered result. Release the profile
    // cursor before issuing the second query on this connection.
    Statement query(
        connection,
        "SELECT ActiveIndex, PageCount FROM WeaponPresetState WHERE CharacterId = ?");
    query.Execute(parameters);
    std::array results{Number(state.active_index), Number(count)};
    present = query.Fetch(results);
  }
  if (present) {
    if (count == 0 || count > 5 || state.active_index >= count) {
      throw std::runtime_error("Invalid stored preset count.");
    }
    state.primary_items.clear();
    Statement slots(
        connection,
        "SELECT PresetIndex, PrimaryItemId FROM WeaponPresetSlots WHERE CharacterId = ? ORDER BY PresetIndex LIMIT 6");
    slots.Execute(parameters);
    std::uint8_t index = 0;
    std::uint32_t item = 0;
    std::array slot_results{Number(index), Number(item)};
    while (slots.Fetch(slot_results)) {
      if (index != state.primary_items.size() ||
          state.primary_items.size() == count) {
        throw std::runtime_error("Invalid stored preset slots.");
      }
      state.primary_items.push_back(item);
    }
    if (state.primary_items.size() != count) {
      throw std::runtime_error("Missing stored preset slot.");
    }
  }
  state.CaptureActive(inventory);
  return state;
}

void PersistPresets(MYSQL* connection, std::uint64_t character_id,
                    const WeaponPresetState& state) {
  state.Validate();
  const auto count = static_cast<std::uint8_t>(state.primary_items.size());
  Statement profile(
      connection,
      "INSERT INTO WeaponPresetState (CharacterId, ActiveIndex, PageCount) VALUES (?, ?, ?) "
      "ON DUPLICATE KEY UPDATE ActiveIndex = ?, PageCount = ?");
  std::array parameters{Number(character_id), Number(state.active_index),
                        Number(count), Number(state.active_index),
                        Number(count)};
  profile.Execute(parameters);
  Statement clear(connection,
                  "DELETE FROM WeaponPresetSlots WHERE CharacterId = ?");
  std::array owner{Number(character_id)};
  clear.Execute(owner);
  for (std::uint8_t index = 0; index != count; ++index) {
    Statement insert(
        connection,
        "INSERT INTO WeaponPresetSlots (CharacterId, PresetIndex, PrimaryItemId) VALUES (?, ?, ?)");
    std::array row{Number(character_id), Number(index),
                   Number(state.primary_items[index])};
    insert.Execute(row);
    if (insert.AffectedRows() != 1) {
      throw std::runtime_error("Preset slot insert failed.");
    }
  }
}

void PersistEquipment(MYSQL* connection, const CharacterInventory& before,
                      CharacterInventory& after) {
  if (before.slots.size() != after.slots.size()) {
    throw std::logic_error("Equipment change cannot grant items.");
  }
  for (std::size_t index = 0; index != before.slots.size(); ++index) {
    const auto& old = before.slots[index];
    auto& changed = after.slots[index];
    if (old.item_id != changed.item_id) {
      throw std::logic_error("Equipment slot identity changed.");
    }
    if (old.equipped == changed.equipped) {
      continue;
    }
    if (old.revision == (std::numeric_limits<std::uint64_t>::max)()) {
      throw std::runtime_error("Equipment revision exhausted.");
    }
    const std::uint8_t equipped = changed.equipped ? 1 : 0;
    const auto revision = old.revision + 1;
    Statement update(connection,
                     "UPDATE InventorySlots SET IsEquipped = ?, Revision = ? "
                     "WHERE CharacterId = ? AND ItemId = ? AND Revision = ?");
    std::array parameters{Number(equipped), Number(revision),
                          Number(before.character_id), Number(old.item_id),
                          Number(old.revision)};
    update.Execute(parameters);
    if (update.AffectedRows() != 1) {
      throw std::runtime_error(
          "Equipment revision changed during transaction.");
    }
    changed.revision = revision;
  }
}

void PersistGrant(MYSQL* connection,
                  const features::characters::CharacterRecord& character,
                  InventorySlot& slot) {
  const auto previous_revision = slot.revision;
  const auto revision =
      previous_revision + 1;  // preparation already checked overflow
  const auto usage = static_cast<std::uint8_t>(slot.usage);
  const auto expiry = slot.expires_at_unix_seconds.value_or(0);
  bool expiry_null = !slot.expires_at_unix_seconds;
  if (previous_revision == 0) {
    protocol::InventoryItemRecord detail;
    detail.text_at36 = slot.text_at36;
    detail.attributes_at24 = slot.attributes_at24;
    const auto bytes =
        protocol::InventoryItemRecord::EncodeList(std::span(&detail, 1));
    const std::uint8_t equipped = slot.equipped ? 1 : 0;
    Statement insert(
        connection,
        "INSERT INTO InventorySlots (CharacterId, ItemId, UsageKind, Quantity, ExpiresAtUnixSeconds, "
        "IsEquipped, DetailVersion, DetailData, Revision) VALUES (?, ?, ?, ?, ?, ?, 1, ?, ?)");
    std::array parameters{
        Number(character.character_id),
        Number(slot.item_id),
        Number(usage),
        Number(slot.quantity),
        Number(expiry),
        Number(equipped),
        Bytes(bytes.data(), static_cast<unsigned long>(bytes.size())),
        Number(revision)};
    parameters[4].is_null = &expiry_null;
    insert.Execute(parameters);
    if (insert.AffectedRows() != 1) {
      throw std::runtime_error("Purchase inventory insert failed.");
    }
  } else {
    // Preserve equipped state and opaque detail; buying never equips.
    Statement update(
        connection,
        "UPDATE InventorySlots SET Quantity = ?, ExpiresAtUnixSeconds = ?, Revision = ? "
        "WHERE CharacterId = ? AND ItemId = ? AND Revision = ? AND UsageKind = ?");
    std::array parameters{Number(slot.quantity), Number(expiry),
                          Number(revision),      Number(character.character_id),
                          Number(slot.item_id),  Number(previous_revision),
                          Number(usage)};
    parameters[1].is_null = &expiry_null;
    update.Execute(parameters);
    if (update.AffectedRows() != 1) {
      throw std::runtime_error("Purchase inventory revision changed.");
    }
  }
  slot.revision = revision;
}

CashInventory LoadCashSlots(MYSQL* connection, std::uint64_t account_id,
                            std::uint64_t character_id) {
  CashInventory inventory{account_id, character_id, {}};
  Statement query(
      connection,
      "SELECT SlotId, StockId, ItemId, UsageKind, Amount, MaxAmount, CashPrice, PurchasedAtUnixSeconds "
      "FROM CashInventorySlots WHERE CharacterId = ? ORDER BY SlotId LIMIT 1025");
  std::array parameters{Number(character_id)};
  query.Execute(parameters);
  CashInventorySlot slot;
  std::uint8_t usage = 0;
  std::array results{
      Number(slot.slot_id),    Number(slot.stock_id),
      Number(slot.item_id),    Number(usage),
      Number(slot.amount),     Number(slot.max_amount),
      Number(slot.cash_price), Number(slot.purchased_at_unix_seconds)};
  while (query.Fetch(results)) {
    if (inventory.slots.size() ==
        protocol::CashInventoryUpdateNotification::kMaxSlots) {
      throw std::runtime_error("Stored cash inventory is oversized.");
    }
    slot.usage = static_cast<InventoryUsage>(usage);
    inventory.slots.push_back(slot);
  }
  (void)inventory.CreateNotification(account_id).Encode();
  return inventory;
}

features::characters::CharacterRecord ReadCurrentCharacter(
    MYSQL* connection, const features::characters::CharacterRecord& character) {
  auto current = character;
  Statement query(
      connection,
      "SELECT CharacterType, ColorIndex, Level, Experience, Lucci, Cash, NameUtf16 FROM Characters WHERE CharacterId = ?");
  std::array parameters{Number(character.character_id)};
  query.Execute(parameters);
  std::array<std::uint8_t, 32> name{};
  unsigned long name_length = 0;
  std::array results{
      Number(current.character_type),
      Number(current.color_index),
      Number(current.statistics.level),
      Number(current.statistics.experience),
      Number(current.statistics.lucci),
      Number(current.statistics.cash),
      Bytes(name.data(), static_cast<unsigned long>(name.size()))};
  results[6].length = &name_length;
  if (!query.Fetch(results) || name_length == 0 || name_length > name.size() ||
      name_length % 2 != 0 ||
      current.character_type != character.character_type ||
      current.color_index != character.color_index) {
    throw std::runtime_error("Inventory transaction character changed.");
  }
  current.character_name.clear();
  for (std::size_t i = 0; i != name_length; i += 2) {
    current.character_name.push_back(static_cast<char16_t>(
        name[i] | (static_cast<std::uint16_t>(name[i + 1]) << 8)));
  }
  if (current.character_name != character.character_name) {
    throw std::runtime_error(
        "Inventory transaction character name changed; reconnect required.");
  }
  return current;
}

}  // namespace bubble_fighter::server::persistence::inventory_sql
