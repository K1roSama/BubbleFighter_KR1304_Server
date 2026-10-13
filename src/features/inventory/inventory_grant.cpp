#include "features/inventory/inventory_grant.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "features/inventory/item_catalog.h"
#include "features/inventory/item_use.h"

namespace bubble_fighter::server::features::inventory {
InventoryGrantStatus InventoryGrant::Apply(
    CharacterInventory& inventory, std::uint32_t item_id, std::uint32_t amount,
    std::uint32_t max_amount, std::uint64_t observed_at_unix_seconds) {
  (void)inventory.ToRecords(inventory.account_id, observed_at_unix_seconds);
  const auto* definition = ItemCatalog::Find(item_id);
  // Owning a catalog item does not execute its special function. ItemUse
  // and EquipmentChange retain their own effect/eligibility validation.
  if (!definition || amount == 0 ||
      (definition->usage != InventoryUsage::kCount &&
       definition->usage != InventoryUsage::kDuration &&
       definition->usage != InventoryUsage::kLife)) {
    throw std::invalid_argument("Unsupported trusted ordinary item grant.");
  }
  auto found =
      std::find_if(inventory.slots.begin(), inventory.slots.end(),
                   [&](const auto& slot) { return slot.item_id == item_id; });
  InventorySlot changed;
  changed.item_id = item_id;
  changed.usage = definition->usage;
  if (found != inventory.slots.end()) {
    if (found->usage != definition->usage) {
      throw std::invalid_argument("Stored grant usage differs from catalog.");
    }
    if (found->usage == InventoryUsage::kDuration &&
        !found->expires_at_unix_seconds) {
      return InventoryGrantStatus::kAlreadyOwned;
    }
    changed = *found;
  }
  if (changed.revision == (std::numeric_limits<std::uint64_t>::max)()) {
    throw std::runtime_error("Grant inventory revision exhausted.");
  }
  if (changed.usage == InventoryUsage::kCount ||
      changed.usage == InventoryUsage::kLife) {
    // Life is carried in the ordinary quantity field (E053). The
    // resource maxDuration is not established as a durability cap.
    const auto limit =
        changed.usage == InventoryUsage::kLife
            ? InventorySlot::kMaxQuantity
            : (std::min)(max_amount, InventorySlot::kMaxQuantity);
    if (changed.quantity > limit || amount > limit - changed.quantity) {
      return InventoryGrantStatus::kLimitExceeded;
    }
    changed.quantity += amount;
  } else if (amount == 0xFFFFFFFFU) {
    changed.quantity = 1;
    changed.expires_at_unix_seconds.reset();
  } else {
    const auto days =
        (std::min<std::uint64_t>)(max_amount,
                                  InventorySlot::kMaxFiniteSeconds / 86'400);
    const auto base = (std::max)(observed_at_unix_seconds,
                                 changed.expires_at_unix_seconds.value_or(
                                     observed_at_unix_seconds));
    const auto seconds = static_cast<std::uint64_t>(amount) * 86'400;
    if (amount > days ||
        base - observed_at_unix_seconds > days * 86'400 - seconds ||
        seconds > InventorySlot::kMaxUnixSeconds - base) {
      return InventoryGrantStatus::kLimitExceeded;
    }
    changed.quantity = 1;
    changed.expires_at_unix_seconds = base + seconds;
  }
  if (found == inventory.slots.end()) {
    inventory.slots.push_back(std::move(changed));
  } else {
    *found = std::move(changed);
  }
  return InventoryGrantStatus::kGranted;
}
}  // namespace bubble_fighter::server::features::inventory
