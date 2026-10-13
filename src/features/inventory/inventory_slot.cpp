#include "features/inventory/inventory_slot.h"

#include <bit>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace bubble_fighter::server::features::inventory {
protocol::InventoryItemRecord InventorySlot::ToProtocol(
    std::uint64_t observed_at_unix_seconds) const {
  if (item_id == 0 || item_id > kMaxItemId || quantity > kMaxQuantity ||
      observed_at_unix_seconds > kMaxUnixSeconds ||
      (usage != InventoryUsage::kCount && usage != InventoryUsage::kDuration &&
       usage != InventoryUsage::kLife) ||
      (expires_at_unix_seconds &&
       (usage != InventoryUsage::kDuration ||
        *expires_at_unix_seconds > kMaxUnixSeconds))) {
    throw std::invalid_argument("Invalid ordinary inventory slot.");
  }

  double remaining_days = 0.0;
  bool exhausted = quantity == 0;
  if (usage == InventoryUsage::kDuration) {
    exhausted = expires_at_unix_seconds &&
                *expires_at_unix_seconds <= observed_at_unix_seconds;
    if (!expires_at_unix_seconds) {
      remaining_days = kPermanentDisplayDays;
    } else if (!exhausted) {
      const auto seconds = *expires_at_unix_seconds - observed_at_unix_seconds;
      if (seconds > kMaxFiniteSeconds) {
        throw std::invalid_argument(
            "Finite inventory duration exceeds the local display limit.");
      }
      remaining_days = static_cast<double>(seconds) / 86'400.0;
    }
  }

  protocol::InventoryItemRecord record;
  record.key = item_id;
  record.value_at4 = quantity;
  static_assert(sizeof(double) == sizeof(std::uint64_t) &&
                std::numeric_limits<double>::is_iec559);
  record.bits_at8 = std::bit_cast<std::uint64_t>(remaining_days);
  record.text_at36 = text_at36;
  record.attributes_at24 = attributes_at24;
  // Ordinary slots only: bits 0/1 are equipped and depleted/expired.
  // Premium and other special flags are intentionally outside this model.
  record.flags =
      static_cast<std::uint8_t>((equipped ? 1 : 0) | (exhausted ? 2 : 0));
  return record;
}

std::vector<protocol::InventoryItemRecord> CharacterInventory::ToRecords(
    std::uint64_t authenticated_account_id,
    std::uint64_t observed_at_unix_seconds) const {
  if (account_id == 0 || account_id != authenticated_account_id ||
      character_id == 0 || character_id > 0xFFFFFFFFULL ||
      observed_at_unix_seconds > InventorySlot::kMaxUnixSeconds ||
      slots.size() > protocol::InventoryItemRecord::kMaxCollectionEntries) {
    throw std::invalid_argument("Invalid inventory identity or size.");
  }
  std::unordered_set<std::uint32_t> ids;
  std::vector<protocol::InventoryItemRecord> records;
  records.reserve(slots.size());
  for (const auto& slot : slots) {
    if (!ids.insert(slot.item_id).second) {
      throw std::invalid_argument("Duplicate inventory item ID.");
    }
    records.push_back(slot.ToProtocol(observed_at_unix_seconds));
  }
  // Enforce the cumulative attribute/string/byte budgets, not just each
  // row's validity. a real empty stored inventory is represented by zero.
  (void)protocol::InventoryItemRecord::EncodeList(records);
  return records;
}

protocol::InventoryUpdateAllNotification CharacterInventory::CreateNotification(
    std::uint64_t authenticated_account_id,
    std::uint64_t observed_at_unix_seconds) const {
  return protocol::InventoryUpdateAllNotification::FromRecords(
      ToRecords(authenticated_account_id, observed_at_unix_seconds));
}
}  // namespace bubble_fighter::server::features::inventory
