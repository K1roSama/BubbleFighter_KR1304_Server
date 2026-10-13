#ifndef BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_CATALOG_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_CATALOG_RESPONSE_H_

#include "features/characters/character_store.h"
#include "protocol/shop/shop_entry_request.h"
#include "protocol/shop/shop_entry_response.h"

namespace bubble_fighter::server::features::shop {
// A full resource-derived snapshot. Session code decides whether the
// client already owns this revision; no account balance or inventory
// is changed by browsing. Purchase policy is a separate operation.
protocol::ShopEntryResponse CreateShopCatalogResponse(
    const protocol::ShopEntryRequest& request,
    const characters::CharacterRecord& character,
    std::uint64_t observed_at_unix_seconds);
}  // namespace bubble_fighter::server::features::shop

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_SHOP_SHOP_CATALOG_RESPONSE_H_
