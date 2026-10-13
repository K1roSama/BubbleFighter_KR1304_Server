#include "features/shop/cash_shop.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "features/characters/character_profile.h"
#include "features/characters/character_statistics.h"
#include "features/characters/character_store.h"
#include "features/inventory/cash_inventory.h"
#include "features/inventory/inventory_grant.h"
#include "features/inventory/inventory_presentation.h"
#include "features/inventory/inventory_slot.h"
#include "features/inventory/inventory_validation.h"
#include "features/inventory/item_catalog.h"
#include "features/shop/shop_catalog.h"
#include "features/shop/shop_stock_catalog.h"
#include "protocol/inventory/cash_inventory_pickup_request.h"
#include "protocol/inventory/cash_inventory_pickup_response.h"
#include "protocol/inventory/inventory_view_update_notification.h"
#include "protocol/shop/shop_purchase_request.h"
#include "protocol/shop/shop_purchase_response.h"

namespace bubble_fighter::server::features::shop {
namespace {
void Validate(const characters::CharacterRecord& character,
              const inventory::CharacterInventory& inventory,
              const inventory::CashInventory& pending,
              std::uint64_t observed_at) {
  if (!inventory::BelongsToCharacter(character, inventory) ||
      pending.account_id != character.account_id ||
      pending.character_id != character.character_id ||
      !inventory::IsValidInventoryCharacter(character)) {
    throw std::invalid_argument("Cash transaction ownership mismatch.");
  }
  (void)inventory.ToRecords(character.account_id, observed_at);
  (void)pending.CreateNotification(character.account_id).Encode();
  if (!inventory::MatchesItemCatalog(inventory)) {
    throw std::invalid_argument("Cash transaction inventory catalog mismatch.");
  }
}
}  // namespace

CashPurchaseResult CashShop::Purchase(
    const protocol::ShopPurchaseRequest& request,
    const characters::CharacterRecord& character,
    const inventory::CharacterInventory& inventory,
    inventory::CashInventory pending, std::uint32_t assigned_slot_id,
    std::uint64_t observed_at_unix_seconds) {
  using namespace inventory;
  using Response = protocol::ShopPurchaseResponse;
  Validate(character, inventory, pending, observed_at_unix_seconds);
  CashPurchaseResult result;
  result.purchase.character = character;
  result.purchase.inventory = inventory;
  result.purchase.response.stock_id = request.stock_id;
  result.pending = std::move(pending);
  const auto refuse = [&](std::uint32_t code) {
    result.purchase.response.result = code;
    result.purchase.packets = {result.purchase.response.Encode()};
    return result;
  };
  const auto* stock = ShopStockCatalog::Find(request.stock_id);
  if (!stock || request.value_at4 != 0) {
    return refuse(Response::kIneligible);
  }
  const auto* offer = ShopCatalog::FindItem(stock->item_id);
  const auto* item = ItemCatalog::Find(stock->item_id);
  // Original E17E80 treats currency kinds 1 (Cash) and 3 (CashCn)
  // against the same cash balance; neither uses the Lucci balance.
  if (!offer || !item ||
      (offer->currency != ShopCurrency::kCash &&
       offer->currency != ShopCurrency::kCashCn) ||
      (item->usage != InventoryUsage::kCount &&
       item->usage != InventoryUsage::kDuration &&
       item->usage != InventoryUsage::kLife) ||
      stock->sell_price > characters::CharacterStatistics::kMaxValue ||
      stock->duration == 0 ||
      (item->usage == InventoryUsage::kDuration &&
       stock->duration != 0xFFFFFFFFU && stock->duration > 10000) ||
      (item->usage != InventoryUsage::kDuration &&
       stock->duration > InventorySlot::kMaxQuantity)) {
    return refuse(Response::kIneligible);
  }
  const auto chunks = item->usage == InventoryUsage::kDuration
                          ? 1U
                          : (stock->duration + 65534U) / 65535U;
  if (chunks > protocol::CashInventoryUpdateNotification::kMaxSlots -
                   result.pending.slots.size()) {
    return refuse(Response::kIneligible);
  }
  const auto max_amount = item->usage == InventoryUsage::kCount
                              ? (std::max)(offer->max_amount, stock->duration)
                              : offer->max_amount;
  // Reserve grant capacity against all pending purchases of this item.
  // This projection is discarded; purchase does not start the duration.
  auto projected = inventory;
  for (const auto& slot : result.pending.slots) {
    if (slot.item_id == stock->item_id &&
        InventoryGrant::Apply(projected, slot.item_id, slot.amount,
                              slot.max_amount, observed_at_unix_seconds) !=
            InventoryGrantStatus::kGranted) {
      return refuse(Response::kIneligible);
    }
  }
  const auto grant =
      InventoryGrant::Apply(projected, stock->item_id, stock->duration,
                            max_amount, observed_at_unix_seconds);
  if (grant == InventoryGrantStatus::kAlreadyOwned) {
    return refuse(Response::kAlreadyOwned);
  }
  if (grant != InventoryGrantStatus::kGranted) {
    return refuse(Response::kIneligible);
  }
  (void)projected.ToRecords(character.account_id, observed_at_unix_seconds);
  if (character.statistics.cash < stock->sell_price) {
    return refuse(Response::kInsufficientCash);
  }
  if (assigned_slot_id == 0 || assigned_slot_id > CashInventory::kMaxSlotId ||
      chunks - 1 > CashInventory::kMaxSlotId - assigned_slot_id ||
      std::any_of(result.pending.slots.begin(), result.pending.slots.end(),
                  [&](const auto& slot) {
                    return slot.slot_id >= assigned_slot_id &&
                           slot.slot_id - assigned_slot_id < chunks;
                  })) {
    throw std::invalid_argument("Invalid trusted cash slot allocation.");
  }
  auto remaining = stock->duration;
  for (std::uint32_t index = 0; index < chunks; ++index) {
    const auto amount = item->usage == InventoryUsage::kDuration
                            ? remaining
                            : (std::min)(remaining, 65535U);
    result.pending.slots.push_back(
        {assigned_slot_id + index, stock->stock_id, stock->item_id, item->usage,
         amount, max_amount, index == 0 ? stock->sell_price : 0U,
         observed_at_unix_seconds});
    remaining -= amount;
  }
  result.cash_cost = stock->sell_price;
  result.purchase.character.statistics.cash -= result.cash_cost;
  result.purchase.response.result = Response::kSuccess;
  result.purchase.response.value_at40 = assigned_slot_id;
  (void)CreateInventoryEquipment(result.purchase.character, inventory,
                                 observed_at_unix_seconds);
  const auto profile = characters::CharacterProfile::BindKnownFields(
      characters::CreateCharacterProfileBaseline(), result.purchase.character,
      character.account_id);
  // S810 must precede S816: immediate C829 and S835 need the pending ID.
  result.purchase.packets = {
      profile.Encode(),
      result.pending.CreateNotification(character.account_id).Encode(),
      result.purchase.response.Encode()};
  return result;
}

CashPickupResult CashShop::Pickup(
    const protocol::CashInventoryPickupRequest& request,
    const characters::CharacterRecord& character,
    inventory::CharacterInventory inventory, inventory::CashInventory pending,
    std::uint64_t observed_at_unix_seconds) {
  using namespace inventory;
  using Response = protocol::CashInventoryPickupResponse;
  Validate(character, inventory, pending, observed_at_unix_seconds);
  CashPickupResult result{character,
                          std::move(inventory),
                          std::move(pending),
                          {request.slot_id, Response::kFailed, 0, {}},
                          0,
                          {}};
  const auto refuse = [&](std::uint32_t code) {
    result.response.result = code;
    result.packets = {
        result.response.Encode(),
        result.pending.CreateNotification(character.account_id).Encode()};
    return result;
  };
  const auto found = std::find_if(
      result.pending.slots.begin(), result.pending.slots.end(),
      [&](const auto& slot) { return slot.slot_id == request.slot_id; });
  if (found == result.pending.slots.end()) {
    return refuse(Response::kFailed);
  }
  auto granted = result.inventory;
  const auto status =
      InventoryGrant::Apply(granted, found->item_id, found->amount,
                            found->max_amount, observed_at_unix_seconds);
  if (status == InventoryGrantStatus::kAlreadyOwned) {
    return refuse(Response::kAlreadyOwned);
  }
  if (status != InventoryGrantStatus::kGranted) {
    return refuse(Response::kFailed);
  }
  result.inventory = std::move(granted);
  result.changed_item_id = found->item_id;
  result.pending.slots.erase(found);
  result.response.result = Response::kSuccess;
  const auto equipment = CreateInventoryEquipment(character, result.inventory,
                                                  observed_at_unix_seconds);
  // S835 looks up and removes the previous cash slot. Send the refreshed
  // pending list AFTER it, or its result dialog cannot find the item.
  result.packets = {
      result.inventory
          .CreateNotification(character.account_id, observed_at_unix_seconds)
          .Encode(),
      protocol::InventoryViewUpdateNotification{equipment}.Encode(),
      result.response.Encode(),
      result.pending.CreateNotification(character.account_id).Encode()};
  return result;
}
}  // namespace bubble_fighter::server::features::shop
