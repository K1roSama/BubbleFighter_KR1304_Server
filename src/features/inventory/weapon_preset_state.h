#ifndef BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_WEAPON_PRESET_STATE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_WEAPON_PRESET_STATE_H_

#include "features/inventory/equipment_view.h"
#include "protocol/inventory/weapon_preset_change_request.h"
#include "protocol/inventory/weapon_preset_update_notification.h"

namespace bubble_fighter::server::features::inventory {
struct WeaponPresetState {
  // User-selected local policy: all five pages initially available.
  // This is not a recovered original account entitlement.
  static constexpr std::size_t kInitialPages = 5;
  std::uint8_t active_index = 0;
  std::vector<std::uint32_t> primary_items =
      std::vector<std::uint32_t>(kInitialPages, 0);

  void Validate() const;
  // Ordinary equipped flags are authoritative for the active page. This
  // also imports existing equipment without granting a starter weapon.
  void CaptureActive(const CharacterInventory& inventory);
  protocol::WeaponPresetUpdateNotification CreateNotification(
      const characters::CharacterRecord& character,
      const CharacterInventory& inventory,
      std::uint64_t observed_at_unix_seconds) const;
  bool operator==(const WeaponPresetState&) const = default;
};

struct WeaponPresetChangeResult {
  CharacterInventory inventory;
  WeaponPresetState presets;
  std::vector<protocol::TcpPacket> packets;
};

WeaponPresetChangeResult PrepareWeaponPresetChange(
    const characters::CharacterRecord& character, CharacterInventory inventory,
    WeaponPresetState presets,
    const protocol::WeaponPresetChangeRequest& request,
    std::uint64_t observed_at_unix_seconds);
}  // namespace bubble_fighter::server::features::inventory

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_INVENTORY_WEAPON_PRESET_STATE_H_
