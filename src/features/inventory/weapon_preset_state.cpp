#include "features/inventory/weapon_preset_state.h"

#include <algorithm>
#include <stdexcept>

#include "features/inventory/item_catalog.h"
#include "protocol/inventory/inventory_view_update_notification.h"
#include "protocol/inventory/weapon_preset_change_response.h"

namespace bubble_fighter::server::features::inventory {
namespace {
constexpr std::uint32_t kPrimaryMask = std::uint32_t{1} << 18;  // E059 sWeapon1
bool IsPrimary(std::uint32_t id) {
  const auto* item = ItemCatalog::Find(id);
  return id > 200000 && id < 300000 && item &&
         item->slot_mask == kPrimaryMask && item->function == 0;
}
}  // namespace

void WeaponPresetState::Validate() const {
  if (primary_items.empty() ||
      primary_items.size() >
          protocol::WeaponPresetChangeRequest::kMaxPresetIndex + 1U ||
      active_index >= primary_items.size()) {
    throw std::invalid_argument("Invalid owned weapon preset pages.");
  }
  for (const auto id : primary_items) {
    if (id != 0 && !IsPrimary(id)) {
      throw std::invalid_argument("Unsupported primary weapon preset item.");
    }
  }
}

void WeaponPresetState::CaptureActive(const CharacterInventory& inventory) {
  Validate();
  std::uint32_t primary = 0;
  for (const auto& slot : inventory.slots) {
    if (!slot.equipped || !IsPrimary(slot.item_id)) {
      continue;
    }
    if (primary != 0) {
      throw std::invalid_argument("Conflicting active primary weapons.");
    }
    primary = slot.item_id;
  }
  primary_items[active_index] = primary;
}

protocol::WeaponPresetUpdateNotification WeaponPresetState::CreateNotification(
    const characters::CharacterRecord& character,
    const CharacterInventory& inventory,
    std::uint64_t observed_at_unix_seconds) const {
  Validate();
  if (inventory.account_id != character.account_id ||
      inventory.character_id != character.character_id ||
      character.character_type >= 32 || !character.statistics.IsValid()) {
    throw std::invalid_argument("Invalid weapon preset owner or character.");
  }
  const auto records =
      inventory.ToRecords(character.account_id, observed_at_unix_seconds);
  protocol::WeaponPresetUpdateNotification result;
  result.byte_at0 = active_index;
  result.player_id = static_cast<std::uint32_t>(character.character_id);
  for (auto id : primary_items) {
    if (id != 0) {
      const auto slot =
          std::find_if(inventory.slots.begin(), inventory.slots.end(),
                       [&](const auto& item) { return item.item_id == id; });
      const auto* definition = ItemCatalog::Find(id);
      if (slot == inventory.slots.end()) {
        throw std::invalid_argument("Preset weapon is not owned.");
      }
      if (slot->usage != definition->usage) {
        throw std::invalid_argument("Preset weapon usage mismatch.");
      }
      // Expired/depleted/ineligible saved weapons become empty for
      // display and selection, not renewed, replaced or re-granted.
      const auto index =
          static_cast<std::size_t>(slot - inventory.slots.begin());
      if (!definition->enabled || (records[index].flags & 2) != 0 ||
          character.statistics.level < definition->required_level ||
          (definition->allowed_characters &
           (std::uint32_t{1} << character.character_type)) == 0) {
        id = 0;
      }
    }
    // E066 only the first member is confirmed as the primary weapon.
    // A29090 accepts zero as an empty preview. Other positions and the
    // trailing byte vector stay explicitly empty in this local subset.
    result.records.push_back({id, 0, 0});
  }
  return result;
}

WeaponPresetChangeResult PrepareWeaponPresetChange(
    const characters::CharacterRecord& character, CharacterInventory inventory,
    WeaponPresetState presets,
    const protocol::WeaponPresetChangeRequest& request,
    std::uint64_t observed_at_unix_seconds) {
  (void)request.Encode();
  presets.CaptureActive(inventory);
  if (request.object_id != protocol::WeaponPresetChangeRequest::kMenuObjectId ||
      request.preset_index >= presets.primary_items.size()) {
    throw std::invalid_argument(
        "Weapon preset is not available in the menu/plaza.");
  }
  const auto available = presets.CreateNotification(character, inventory,
                                                    observed_at_unix_seconds);
  // Persist cleanup of unavailable selections with the equipment change.
  for (std::size_t i = 0; i != available.records.size(); ++i) {
    presets.primary_items[i] = available.records[i][0];
  }
  presets.active_index = request.preset_index;
  const auto selected = presets.primary_items[presets.active_index];
  for (auto& slot : inventory.slots) {
    if (IsPrimary(slot.item_id)) {
      slot.equipped = slot.item_id == selected;
    }
  }
  const auto view =
      EquipmentView::Create(character, inventory, observed_at_unix_seconds);
  std::vector<protocol::TcpPacket> packets;
  packets.push_back(
      inventory
          .CreateNotification(character.account_id, observed_at_unix_seconds)
          .Encode());
  packets.push_back(protocol::InventoryViewUpdateNotification{view}.Encode());
  packets.push_back(
      presets.CreateNotification(character, inventory, observed_at_unix_seconds)
          .Encode());
  packets.push_back(protocol::WeaponPresetChangeResponse{
      request.preset_index, protocol::WeaponPresetChangeResponse::kSuccess}
                        .Encode());
  return {std::move(inventory), std::move(presets), std::move(packets)};
}
}  // namespace bubble_fighter::server::features::inventory
