#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include "features/characters/character_store.h"
#include "features/inventory/equipment_change.h"
#include "features/inventory/item_use.h"
#include "features/inventory/weapon_preset_state.h"
#include "persistence/inventory/inventory_rows.h"
#include "persistence/my_sql_commands.h"
#include "persistence/my_sql_connection.h"
#include "persistence/my_sql_inventory_store.h"
#include "protocol/codec/packet_writer.h"
#include "protocol/inventory/inventory_item_record.h"
#include "protocol/inventory/item_use_request.h"
#include "protocol/inventory/weapon_preset_change_request.h"

namespace bubble_fighter::server::persistence {
using namespace features::inventory;
using namespace my_sql_commands;
using namespace inventory_sql;
ItemUseResult MySqlInventoryStore::UseItem(
    const features::characters::CharacterRecord& character,
    const protocol::ItemUseRequest& request,
    std::uint64_t observed_at_unix_seconds) {
  auto* connection = connection_.Handle();
  Transaction transaction(connection);
  if (!LockOwner(connection, character.account_id, character.character_id)) {
    throw std::runtime_error("Item use owner is unavailable.");
  }
  const auto current = ReadCurrentCharacter(connection, character);
  const auto before =
      LoadSlots(connection, character.account_id, character.character_id);
  auto result = ItemUse::PrepareRename(current, before, request,
                                       observed_at_unix_seconds);
  if (result.response.result != protocol::ItemUseResponse::kRenameSuccess) {
    return result;
  }
  protocol::PacketWriter name;
  for (const auto unit : result.character.character_name) {
    name.WriteUInt16(static_cast<std::uint16_t>(unit));
  }
  const auto bytes = name.TakeBytes();
  try {
    Statement rename(
        connection,
        "UPDATE Characters SET NameUtf16 = ? WHERE CharacterId = ? AND AccountId = ?");
    std::array parameters{
        Bytes(bytes.data(), static_cast<unsigned long>(bytes.size())),
        Number(character.character_id), Number(character.account_id)};
    rename.Execute(parameters);
    if (rename.AffectedRows() != 1) {
      throw std::runtime_error("Character rename did not update one row.");
    }
  } catch (const MySqlError& error) {
    // The unique UTF-16 byte index arbitrates concurrent different
    // owners without a racy pre-check. Transaction destruction rolls
    // back before the caller can publish the refusal.
    if (error.Code() == 1062) {
      return ItemUse::PrepareRename(current, before, request,
                                    observed_at_unix_seconds, true);
    }
    throw;
  }
  auto& slot = *std::find_if(
      result.inventory.slots.begin(), result.inventory.slots.end(),
      [&](const auto& value) { return value.item_id == request.item_id; });
  // Loaded slots always have a nonzero revision. The complete response
  // batch was encoded before either write; a later failure rolls back both.
  PersistGrant(connection, current, slot);
  transaction.Commit();
  return result;
}

EquipmentChangeResult MySqlInventoryStore::ChangeEquipment(
    const features::characters::CharacterRecord& character,
    std::uint64_t item_identifier, bool equip,
    std::uint64_t observed_at_unix_seconds) {
  auto* connection = connection_.Handle();
  Transaction transaction(connection);
  if (!LockOwner(connection, character.account_id, character.character_id)) {
    throw std::runtime_error("Equipment owner is unavailable.");
  }
  {
    // The session profile may predate another trusted writer. Do not
    // authorize equipment against stale model, color or level values.
    Statement query(
        connection,
        "SELECT CharacterType, ColorIndex, Level FROM Characters WHERE CharacterId = ?");
    std::array parameters{Number(character.character_id)};
    query.Execute(parameters);
    std::uint32_t type = 0, color = 0, level = 0;
    std::array results{Number(type), Number(color), Number(level)};
    if (!query.Fetch(results) || type != character.character_type ||
        color != character.color_index || level != character.statistics.level) {
      throw std::runtime_error("Equipment character profile changed.");
    }
  }
  const auto before =
      LoadSlots(connection, character.account_id, character.character_id);
  auto prepared = EquipmentChange::Prepare(character, before, item_identifier,
                                           equip, observed_at_unix_seconds);
  auto presets = LoadPresets(connection, before);
  presets.CaptureActive(prepared.inventory);
  prepared.packets.push_back(presets
                                 .CreateNotification(character,
                                                     prepared.inventory,
                                                     observed_at_unix_seconds)
                                 .Encode());
  PersistEquipment(connection, before, prepared.inventory);
  PersistPresets(connection, character.character_id, presets);
  transaction.Commit();
  return prepared;
}

WeaponPresetChangeResult MySqlInventoryStore::ChangeWeaponPreset(
    const features::characters::CharacterRecord& character,
    const protocol::WeaponPresetChangeRequest& request,
    std::uint64_t observed_at_unix_seconds) {
  auto* connection = connection_.Handle();
  Transaction transaction(connection);
  if (!LockOwner(connection, character.account_id, character.character_id)) {
    throw std::runtime_error("Weapon preset owner is unavailable.");
  }
  const auto current = ReadCurrentCharacter(connection, character);
  const auto before =
      LoadSlots(connection, character.account_id, character.character_id);
  auto prepared = PrepareWeaponPresetChange(current, before,
                                            LoadPresets(connection, before),
                                            request, observed_at_unix_seconds);
  // The complete batch is already encoded. A failure in either write
  // rolls back both the equipment revisions and all preset state.
  PersistEquipment(connection, before, prepared.inventory);
  PersistPresets(connection, character.character_id, prepared.presets);
  transaction.Commit();
  return prepared;
}

bool MySqlInventoryStore::Save(std::uint64_t authenticated_account_id,
                               std::uint64_t character_id, InventorySlot& slot,
                               std::uint64_t observed_at_unix_seconds) {
  (void)slot.ToProtocol(observed_at_unix_seconds);
  if (slot.revision == (std::numeric_limits<std::uint64_t>::max)()) {
    throw std::invalid_argument("Inventory revision exhausted.");
  }
  auto* connection = connection_.Handle();
  Transaction transaction(connection);
  if (!LockOwner(connection, authenticated_account_id, character_id)) {
    return false;
  }
  auto inventory =
      LoadSlots(connection, authenticated_account_id, character_id);
  const auto found = std::find_if(
      inventory.slots.begin(), inventory.slots.end(),
      [&](const auto& current) { return current.item_id == slot.item_id; });
  if (found == inventory.slots.end()) {
    if (slot.revision != 0) {
      return false;
    }
    inventory.slots.push_back(slot);
  } else {
    if (slot.revision == 0 || found->revision != slot.revision) {
      return false;
    }
    if (found->usage != slot.usage) {
      throw std::invalid_argument(
          "Inventory usage is a catalog property, not a mutable slot value.");
    }
    *found = slot;
  }
  (void)inventory.ToRecords(authenticated_account_id, observed_at_unix_seconds);

  protocol::InventoryItemRecord detail;
  detail.text_at36 = slot.text_at36;
  detail.attributes_at24 = slot.attributes_at24;
  const auto bytes =
      protocol::InventoryItemRecord::EncodeList(std::span(&detail, 1));
  const auto usage = static_cast<std::uint8_t>(slot.usage);
  const std::uint8_t equipped = slot.equipped ? 1 : 0;
  const auto expiry = slot.expires_at_unix_seconds.value_or(0);
  bool expiry_null = !slot.expires_at_unix_seconds;
  const auto revision = slot.revision + 1;
  if (slot.revision == 0) {
    Statement insert(
        connection,
        "INSERT INTO InventorySlots (UsageKind, Quantity, ExpiresAtUnixSeconds, IsEquipped, "
        "DetailData, Revision, CharacterId, ItemId, DetailVersion) VALUES (?, ?, ?, ?, ?, ?, ?, ?, 1)");
    std::array parameters{
        Number(usage),
        Number(slot.quantity),
        Number(expiry),
        Number(equipped),
        Bytes(bytes.data(), static_cast<unsigned long>(bytes.size())),
        Number(revision),
        Number(character_id),
        Number(slot.item_id)};
    parameters[2].is_null = &expiry_null;
    insert.Execute(parameters);
    if (insert.AffectedRows() != 1) {
      throw std::runtime_error("Inventory insert did not affect one row.");
    }
  } else {
    Statement update(
        connection,
        "UPDATE InventorySlots SET UsageKind = ?, Quantity = ?, ExpiresAtUnixSeconds = ?, "
        "IsEquipped = ?, DetailData = ?, Revision = ?, DetailVersion = 1 WHERE CharacterId = ? AND ItemId = ? AND Revision = ?");
    std::array parameters{
        Number(usage),
        Number(slot.quantity),
        Number(expiry),
        Number(equipped),
        Bytes(bytes.data(), static_cast<unsigned long>(bytes.size())),
        Number(revision),
        Number(character_id),
        Number(slot.item_id),
        Number(slot.revision)};
    parameters[2].is_null = &expiry_null;
    update.Execute(parameters);
    if (update.AffectedRows() != 1) {
      return false;
    }
  }
  transaction.Commit();
  slot.revision = revision;
  return true;
}
}  // namespace bubble_fighter::server::persistence
