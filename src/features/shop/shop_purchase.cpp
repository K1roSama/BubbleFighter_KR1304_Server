#include "features/shop/shop_purchase.h"

#include <cstdint>
#include <stdexcept>
#include <utility>

#include "features/characters/character_profile.h"
#include "features/characters/character_statistics.h"
#include "features/characters/character_store.h"
#include "features/inventory/inventory_grant.h"
#include "features/inventory/inventory_presentation.h"
#include "features/inventory/inventory_slot.h"
#include "features/inventory/inventory_validation.h"
#include "features/inventory/item_catalog.h"
#include "features/shop/shop_catalog.h"
#include "features/shop/shop_stock_catalog.h"
#include "protocol/inventory/inventory_view_update_notification.h"
#include "protocol/shop/shop_purchase_request.h"
#include "protocol/shop/shop_purchase_response.h"

namespace bubble_fighter::server::features::shop {
ShopPurchaseResult ShopPurchase::Prepare(
    const protocol::ShopPurchaseRequest& request,
    const characters::CharacterRecord& character,
    inventory::CharacterInventory inventory,
    std::uint64_t observed_at_unix_seconds) {
  using namespace inventory;
  using Response = protocol::ShopPurchaseResponse;
  // Validate trusted state even for a refused offer. Corrupt state is a
  // server failure, not a successful purchase or an invented error code.
  if (!BelongsToCharacter(character, inventory) ||
      !IsValidInventoryCharacter(character)) {
    throw std::invalid_argument("Purchase inventory ownership is invalid.");
  }
  (void)inventory.ToRecords(character.account_id, observed_at_unix_seconds);
  if (!MatchesItemCatalog(inventory)) {
    throw std::invalid_argument(
        "Purchase inventory does not match the current item catalog.");
  }
  // Render the equipment view after applying the grant: a just-expired
  // equipped target can become valid again through this renewal.
  ShopPurchaseResult result;
  result.character = character;
  result.inventory = std::move(inventory);
  result.response.stock_id = request.stock_id;
  const auto refuse = [&](std::uint32_t status) {
    result.response.result = status;
    result.packets = {result.response.Encode()};
    return result;
  };
  const auto* stock = ShopStockCatalog::Find(request.stock_id);
  if (request.value_at4 != 0 || !stock) {
    return refuse(Response::kIneligible);
  }
  const auto* offer = ShopCatalog::FindItem(stock->item_id);
  const auto* definition = ItemCatalog::Find(stock->item_id);
  // All resource and fallback Stock entries of a published item are offers.
  // Looking them up directly avoids rebuilding a 1 MiB catalog per buy.
  if (!offer || !definition || offer->currency != ShopCurrency::kLucci ||
      (definition->usage != InventoryUsage::kCount &&
       definition->usage != InventoryUsage::kDuration &&
       definition->usage != InventoryUsage::kLife) ||
      stock->sell_price > characters::CharacterStatistics::kMaxValue ||
      stock->duration == 0) {
    return refuse(Response::kIneligible);
  }

  auto granted = result.inventory;
  const auto grant =
      InventoryGrant::Apply(granted, stock->item_id, stock->duration,
                            offer->max_amount, observed_at_unix_seconds);
  if (grant == InventoryGrantStatus::kAlreadyOwned) {
    return refuse(Response::kAlreadyOwned);
  }
  if (grant != InventoryGrantStatus::kGranted) {
    return refuse(Response::kIneligible);
  }
  if (character.statistics.lucci < stock->sell_price) {
    return refuse(Response::kInsufficientLucci);
  }
  result.inventory = std::move(granted);
  result.character.statistics.lucci -= stock->sell_price;
  result.lucci_cost = stock->sell_price;
  result.changed_item_id = stock->item_id;
  result.response.result = Response::kSuccess;
  // Ordinary Lucci UI: no special-shop selector or cash pickup identity.
  // Opaque zero/empty fields are an explicit local baseline, not claimed
  // original-server defaults. Updates precede the modal success dialog.
  const auto equipment = CreateInventoryEquipment(
      result.character, result.inventory, observed_at_unix_seconds);
  const auto profile = characters::CharacterProfile::BindKnownFields(
      characters::CreateCharacterProfileBaseline(), result.character,
      character.account_id);
  result.packets = {
      profile.Encode(),
      result.inventory
          .CreateNotification(character.account_id, observed_at_unix_seconds)
          .Encode(),
      protocol::InventoryViewUpdateNotification{equipment}.Encode(),
      result.response.Encode()};
  return result;
}
}  // namespace bubble_fighter::server::features::shop
