#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_ENTRY_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_ENTRY_RESPONSE_H_

#include <string>

#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::protocol {
// E061: native Stock record is 28 bytes; its explicit wire form is 26.
struct ShopPriceRecord {
  std::uint32_t stock_id = 0;
  std::uint32_t item_id = 0;
  // Resource isOnSale is NOT mapped here by 0x00A14C20. E062:
  // 0x00E157D0 skips price choices with this flag cleared.
  bool flag_at12 = false;
  std::uint16_t type = 0;
  std::uint32_t duration = 0;
  std::uint32_t show_price = 0;
  std::uint32_t sell_price = 0;
  bool operator==(const ShopPriceRecord&) const = default;
};

struct ShopItemCardRecord {
  std::uint32_t identifier = 0;
  std::uint32_t kind = 0;
  std::u16string name;
  std::uint32_t value_at36 = 0;  // E062: card display-order comparator
  std::uint32_t main_category = 0;
  std::uint32_t sub_category = 0;
  bool flag_at48 = false;  // E062: card visibility gate in 0x0064E210
  std::uint64_t wide_at56 = 0;
  std::uint64_t wide_at64 = 0;
  ShopPriceRecord price;
  std::vector<ShopPriceRecord> prices;
  bool operator==(const ShopItemCardRecord&) const = default;
};

struct ShopSubCategoryRecord {
  std::uint32_t identifier = 0;
  std::u16string name;
  std::uint32_t value_at32 = 0;  // subcategory display order
  std::uint32_t value_at36 =
      0;  // E062: 0=subcategory, 1=promotion, 2=main-category page
  bool operator==(const ShopSubCategoryRecord&) const = default;
};

struct ShopMainCategoryRecord {
  std::uint32_t identifier = 0;
  std::u16string name;
  // 0x00646600 orders categories by native +32.
  std::uint32_t display_order = 0;
  std::vector<ShopSubCategoryRecord> sub_categories;
  bool operator==(const ShopMainCategoryRecord&) const = default;
};

struct ShopEntryResponse {
  static constexpr std::uint16_t kMessageId = 800;
  // Local resource limits, not original server constants.
  static constexpr std::size_t kMaxCollectionEntries = 8192;
  static constexpr std::size_t kMaxTotalCollectionEntries = 32768;
  // Full pinned catalog is about 1.1 MiB. Keep the larger allowance local
  // to S800; inbound TCP decoding retains its existing 1 MiB bound.
  static constexpr std::size_t kMaxPayloadBytes = 2 * 1024 * 1024;
  // The first value is NOT established as a success/result code.
  std::uint32_t value_at0 = 0;
  bool refresh_catalog = false;
  std::uint64_t catalog_revision = 0;
  std::vector<ShopItemCardRecord> items;
  std::uint64_t wide_at32 = 0;
  std::vector<ShopMainCategoryRecord> categories;
  std::vector<std::uint32_t> values_at56;
  std::uint32_t selected_category = 0;

  // All fields are present even when RefreshCatalog is false.
  TcpPacket Encode() const;
  static ShopEntryResponse Decode(const TcpPacket& packet);
  bool operator==(const ShopEntryResponse&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SHOP_SHOP_ENTRY_RESPONSE_H_
