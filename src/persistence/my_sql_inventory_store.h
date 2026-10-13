#ifndef BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_INVENTORY_STORE_H_
#define BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_INVENTORY_STORE_H_

#include "features/inventory/inventory_store.h"
#include "features/inventory/item_use.h"
#include "features/inventory/weapon_preset_state.h"
#include "features/shop/cash_shop.h"
#include "features/shop/shop_purchase.h"
#include "persistence/my_sql_connection.h"

namespace bubble_fighter::server::persistence {
struct InventorySnapshot {
  features::inventory::CharacterInventory inventory;
  features::inventory::CashInventory cash;
  features::inventory::WeaponPresetState presets;
};

class MySqlInventoryStore final : public features::inventory::InventoryStore {
 public:
  explicit MySqlInventoryStore(const std::filesystem::path& configuration);
  std::optional<features::inventory::CharacterInventory> Load(
      std::uint64_t authenticated_account_id, std::uint64_t character_id,
      std::uint64_t observed_at_unix_seconds) override;
  bool Save(std::uint64_t authenticated_account_id, std::uint64_t character_id,
            features::inventory::InventorySlot& slot,
            std::uint64_t observed_at_unix_seconds) override;
  features::inventory::EquipmentChangeResult ChangeEquipment(
      const features::characters::CharacterRecord& character,
      std::uint64_t item_identifier, bool equip,
      std::uint64_t observed_at_unix_seconds) override;
  features::shop::ShopPurchaseResult Purchase(
      const features::characters::CharacterRecord& character,
      const protocol::ShopPurchaseRequest& request,
      std::uint64_t observed_at_unix_seconds);

  std::optional<InventorySnapshot> LoadSnapshot(
      std::uint64_t authenticated_account_id, std::uint64_t character_id,
      std::uint64_t observed_at_unix_seconds);
  // Atomically fills missing starter weapons/current character before the first
  // menu/refresh response. Also repairs roles created by older versions.
  std::optional<InventorySnapshot> LoadMenuSnapshot(
      const features::characters::CharacterRecord& character,
      std::uint64_t observed_at_unix_seconds);
  features::shop::CashPickupResult Pickup(
      const features::characters::CharacterRecord& character,
      const protocol::CashInventoryPickupRequest& request,
      std::uint64_t observed_at_unix_seconds);
  features::inventory::WeaponPresetChangeResult ChangeWeaponPreset(
      const features::characters::CharacterRecord& character,
      const protocol::WeaponPresetChangeRequest& request,
      std::uint64_t observed_at_unix_seconds);
  features::inventory::ItemUseResult UseItem(
      const features::characters::CharacterRecord& character,
      const protocol::ItemUseRequest& request,
      std::uint64_t observed_at_unix_seconds);

 private:
  MySqlConnection connection_;
};
}  // namespace bubble_fighter::server::persistence

#endif  // BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_INVENTORY_STORE_H_
