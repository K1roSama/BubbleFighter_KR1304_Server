#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "features/characters/character_creation.h"
#include "features/inventory/inventory_refresh.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/inventory/cash_inventory_pickup_request.h"
#include "protocol/inventory/equip_item_request.h"
#include "protocol/inventory/inventory_refresh_request.h"
#include "protocol/inventory/item_use_request.h"
#include "protocol/inventory/unequip_item_request.h"
#include "protocol/inventory/weapon_preset_change_request.h"
#include "protocol/shop/cash_balance_request.h"
#include "protocol/shop/cash_balance_response.h"
#include "protocol/shop/shop_entry_request.h"
#include "protocol/shop/shop_purchase_request.h"
#include "sessions/player_session.h"

namespace bubble_fighter::server::sessions {
std::optional<std::vector<protocol::TcpPacket>> PlayerSession::ProcessCommerce(
    const protocol::TcpPacket& packet) {
  using namespace protocol;
  if (packet.message_id == ShopEntryRequest::kMessageId) {
    const auto request = ShopEntryRequest::Decode(packet);
    if (!CommerceInitialized() || !menu_data_ || !shop_catalog_ ||
        !account_id_ || !character_ || !lobby_ ||
        character_->account_id != *account_id_) {
      throw std::runtime_error(
          "Shop entry before full authenticated initialization.");
    }
    auto response =
        shop_catalog_(request, *character_, InventoryObservationTime());
    if (!response.refresh_catalog || response.catalog_revision == 0) {
      throw std::runtime_error(
          "Shop provider must supply a complete versioned snapshot.");
    }
    auto encoded =
        response.Encode();  // validate before acknowledging any cache hit
    if (shop_revision_sent_ == response.catalog_revision &&
        request.catalog_revision == response.catalog_revision) {
      response.refresh_catalog = false;
      response.items.clear();
      response.categories.clear();
      encoded = response.Encode();
    }
    // A client-provided version cannot suppress the first snapshot
    // in this connection. Scene/menu state and inventory stay intact.
    shop_revision_sent_ = response.catalog_revision;
    return std::vector<protocol::TcpPacket>{std::move(encoded)};
  }
  if (packet.message_id == ShopPurchaseRequest::kMessageId) {
    const auto request = ShopPurchaseRequest::Decode(packet);
    if (!CommerceInitialized() || !menu_data_ || !shop_purchase_ ||
        !account_id_ || !character_ || !lobby_ ||
        character_->account_id != *account_id_) {
      throw std::runtime_error(
          "Purchase before full authenticated initialization.");
    }
    auto result =
        shop_purchase_(*character_, request, InventoryObservationTime());
    if (result.character.account_id != *account_id_ ||
        result.character.character_id != character_->character_id) {
      throw std::runtime_error(
          "Purchase provider changed authenticated identity.");
    }
    // The provider returns only after a complete commit or refusal.
    // Keep the latest trusted balances for later menu/plaza reloads.
    character_ = std::move(result.character);
    if (room_) {
      room_.RefreshCharacter(*character_);
    }
    return std::move(result.packets);
  }
  if (packet.message_id == CashBalanceRequest::kMessageId) {
    CashBalanceRequest::Decode(packet);
    if ((state_ != PlayerSessionState::kMenuInitializationSent &&
         !(state_ == PlayerSessionState::kInRoom && room_)) ||
        !menu_data_ || !characters_ || !account_id_ || !character_ || !lobby_ ||
        character_->account_id != *account_id_) {
      throw std::runtime_error(
          "Cash balance query before full authenticated initialization.");
    }
    // E090: the native room UI also sends this read-only query.
    // Reload on every request. A payment on another connection may
    // have changed Cash since this session's last character read.
    auto current = characters_->FindByAccount(*account_id_);
    if (!current || current->account_id != *account_id_ ||
        current->character_id != character_->character_id ||
        current->character_name != character_->character_name ||
        current->character_type != character_->character_type ||
        current->color_index != character_->color_index ||
        !current->statistics.IsValid()) {
      throw std::runtime_error(
          "Cash balance source no longer matches the authenticated character.");
    }
    // The local service has one Cash balance. Unknown fields use
    // an explicit zero baseline, not recovered original defaults
    // or a made-up success code. No balance/item is granted here.
    auto response =
        CashBalanceResponse{0, current->statistics.cash, 0}.Encode();
    character_ = std::move(current);
    if (room_) {
      room_.RefreshCharacter(*character_);
    }
    return std::vector<protocol::TcpPacket>{std::move(response)};
  }
  if (packet.message_id == CashInventoryPickupRequest::kMessageId) {
    const auto request = CashInventoryPickupRequest::Decode(packet);
    if (!CommerceInitialized() || !menu_data_ || !cash_pickup_ ||
        !account_id_ || !character_ || !lobby_ ||
        character_->account_id != *account_id_) {
      throw std::runtime_error(
          "Cash pickup before full authenticated initialization.");
    }
    auto result =
        cash_pickup_(*character_, request, InventoryObservationTime());
    if (result.character.account_id != *account_id_ ||
        result.character.character_id != character_->character_id) {
      throw std::runtime_error(
          "Cash pickup provider changed authenticated identity.");
    }
    character_ = std::move(result.character);
    if (room_) {
      room_.RefreshCharacter(*character_);
    }
    return std::move(result.packets);
  }
  if (packet.message_id == ItemUseRequest::kMessageId) {
    const auto request = ItemUseRequest::Decode(packet);
    if (!CommerceInitialized() || !menu_data_ || !item_use_ || !account_id_ ||
        !character_ || !lobby_ || character_->account_id != *account_id_) {
      throw std::runtime_error(
          "Item use before full authenticated initialization.");
    }
    return ChangeInventory([&] {
      auto result = item_use_(*character_, request, InventoryObservationTime());
      if (result.character.account_id != *account_id_ ||
          result.character.character_id != character_->character_id ||
          result.character.character_type != character_->character_type ||
          result.character.color_index != character_->color_index ||
          !result.character.statistics.IsValid() ||
          features::characters::CharacterCreation::ValidateName(
              result.character.character_name) != 0) {
        throw std::runtime_error(
            "Item use provider changed authenticated identity.");
      }
      character_ = std::move(result.character);
      return std::move(result.packets);
    });
  }
  if (packet.message_id == WeaponPresetChangeRequest::kMessageId) {
    const auto request = WeaponPresetChangeRequest::Decode(packet);
    if (!CommerceInitialized() || !menu_data_ || !weapon_preset_ ||
        !account_id_ || !character_ || !lobby_ ||
        character_->account_id != *account_id_ ||
        request.object_id != WeaponPresetChangeRequest::kMenuObjectId) {
      throw std::runtime_error(
          "Weapon preset change outside initialized menu/plaza.");
    }
    return ChangeInventory([&] {
      return weapon_preset_(*character_, request, InventoryObservationTime());
    });
  }
  if (packet.message_id == EquipItemRequest::kMessageId ||
      packet.message_id == UnequipItemRequest::kMessageId) {
    const bool equip = packet.message_id == EquipItemRequest::kMessageId;
    const auto item = equip
                          ? EquipItemRequest::Decode(packet).item_identifier
                          : UnequipItemRequest::Decode(packet).item_identifier;
    if (!CommerceInitialized() || !menu_data_ || !equipment_change_ ||
        !account_id_ || !character_ || !lobby_ ||
        character_->account_id != *account_id_) {
      throw std::runtime_error(
          "Equipment change before full authenticated initialization.");
    }
    // Installed storage prepares both updates before committing all
    // changed flags. No guessed success code, S3 or scene reset.
    return ChangeInventory([&] {
      return equipment_change_(*character_, item, equip,
                               InventoryObservationTime());
    });
  }
  if (packet.message_id == InventoryRefreshRequest::kMessageId) {
    InventoryRefreshRequest::Decode(packet);
    if (state_ != PlayerSessionState::kMenuInitializationSent || !menu_data_ ||
        !account_id_ || !character_ || !lobby_) {
      throw std::runtime_error(
          "Inventory refresh before full authenticated initialization.");
    }
    // Reload persistence for every explicit refresh. Reuse the
    // trusted menu data source without replaying menu initialization
    // or changing the established plaza/menu context.
    const auto observed_at = InventoryObservationTime();
    const auto data = menu_data_(*character_, observed_at);
    auto packets = features::inventory::InventoryRefresh::Prepare(
        *character_, *account_id_, data.inventory, data.equipment, data.presets,
        observed_at);
    if (data.cash) {
      if (data.cash->character_id != character_->character_id) {
        throw std::runtime_error("Refresh cash owner mismatch.");
      }
      packets.push_back(data.cash->CreateNotification(*account_id_).Encode());
    }
    return packets;
  }
  return std::nullopt;
}
}  // namespace bubble_fighter::server::sessions
