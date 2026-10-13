#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_SLOT_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_SLOT_H_

#include <optional>

#include "protocol/inventory/inventory_update_all_notification.h"

namespace bubble_fighter::server::features::inventory {
// E053: client catalog +208 selects these three display/use branches.
// The caller must obtain this from a verified catalog, not guess by item ID.
enum class InventoryUsage : std::uint8_t {
  kUnknown = 0,
  kCount = 1,
  kDuration = 2,
  kLife = 3
};

struct InventorySlot {
  static constexpr std::uint32_t kMaxItemId = 99'999'999;
  static constexpr std::uint32_t kMaxQuantity = 0x7FFFFFFF;
  static constexpr std::uint64_t kMaxUnixSeconds = 253'402'300'799;
  static constexpr std::uint64_t kMaxFiniteSeconds = 10'000ULL * 86'400;
  // Local representation, supported by the client's >10000-day display
  // branch. This value is not evidence of the original server's policy.
  static constexpr double kPermanentDisplayDays = 36'500.0;

  std::uint32_t item_id = 0;
  InventoryUsage usage = InventoryUsage::kUnknown;
  std::uint32_t quantity = 0;
  // UTC Unix seconds; null means permanent only for Duration items.
  std::optional<std::uint64_t> expires_at_unix_seconds;
  bool equipped = false;
  // Retain unknown detail fields without assigning invented meanings.
  std::u16string text_at36;
  std::vector<protocol::InventoryAttributeRecord> attributes_at24;
  // Local optimistic concurrency version; never serialized to the game.
  std::uint64_t revision = 0;

  protocol::InventoryItemRecord ToProtocol(
      std::uint64_t observed_at_unix_seconds) const;
  bool operator==(const InventorySlot&) const = default;
};

struct CharacterInventory {
  std::uint64_t account_id = 0;
  std::uint64_t character_id = 0;
  std::vector<InventorySlot> slots;

  std::vector<protocol::InventoryItemRecord> ToRecords(
      std::uint64_t authenticated_account_id,
      std::uint64_t observed_at_unix_seconds) const;
  protocol::InventoryUpdateAllNotification CreateNotification(
      std::uint64_t authenticated_account_id,
      std::uint64_t observed_at_unix_seconds) const;
};
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_INVENTORY_SLOT_H_
