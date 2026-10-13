#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_SLOT_RECORD_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_SLOT_RECORD_H_

#include <cstdint>
#include <string>

namespace bubble_fighter::server::protocol {
// BFCLIENT02 CashInvenSlot, native size 52. Wire size is variable;
// no vtable or native string object is transmitted.
struct CashInventorySlotRecord {
  std::uint32_t slot_id = 0;        // native +4, pickup/map key
  std::uint32_t value_at8 = 0;      // unresolved
  std::uint16_t quantity = 0;       // native +12
  std::uint16_t duration_days = 0;  // native +14; >10000 displays permanent
  std::uint32_t stock_id = 0;       // native +16, purchase/result dialog
  std::uint32_t item_id = 0;        // native +20, ordinary item resource lookup
  std::u16string text_at24;         // unresolved
  std::uint8_t byte_at48 = 0;       // 1 exposes cancellation UI
  bool operator==(const CashInventorySlotRecord&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_INVENTORY_CASH_INVENTORY_SLOT_RECORD_H_
