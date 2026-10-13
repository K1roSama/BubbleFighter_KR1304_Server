#include "features/inventory/item_use.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "features/characters/character_creation.h"

namespace bubble_fighter::server::features::inventory {
bool ItemUse::SupportsRename(const ItemDefinition& item) noexcept {
  // Current resource has two ordinary Count cards. The Duration card
  // 190488 has a different naming UI and is deliberately not enabled.
  return (item.item_id == 100104 || item.item_id == 103368) && item.enabled &&
         item.function == 3 && item.parameter == 0 &&
         item.usage == InventoryUsage::kCount;
}

ItemUseResult ItemUse::PrepareRename(
    const characters::CharacterRecord& character, CharacterInventory inventory,
    const protocol::ItemUseRequest& request,
    std::uint64_t observed_at_unix_seconds, bool name_taken) {
  using Response = protocol::ItemUseResponse;
  (void)request.Encode();
  if (inventory.account_id != character.account_id ||
      inventory.character_id != character.character_id ||
      !character.statistics.IsValid() ||
      characters::CharacterCreation::ValidateName(character.character_name) !=
          0 ||
      !characters::CharacterCreation::SupportsSelection(
          character.character_type, character.color_index)) {
    throw std::invalid_argument("Item use owner/profile mismatch.");
  }
  (void)inventory.ToRecords(character.account_id, observed_at_unix_seconds);
  const auto* item = ItemCatalog::Find(request.item_id);
  if (!item || !SupportsRename(*item) || request.value_at4 != 1 ||
      request.arguments.size() != 1 ||
      character.statistics.level < item->required_level) {
    throw std::invalid_argument("Unsupported item use operation.");
  }
  ItemUseResult result{
      character, std::move(inventory), {0, request.item_id, {}}, {}};
  auto slot = std::find_if(
      result.inventory.slots.begin(), result.inventory.slots.end(),
      [&](const auto& row) { return row.item_id == request.item_id; });
  if (slot == result.inventory.slots.end() ||
      slot->usage != InventoryUsage::kCount || slot->quantity == 0 ||
      slot->equipped ||
      slot->revision == (std::numeric_limits<std::uint64_t>::max)()) {
    throw std::invalid_argument("Rename card is unavailable.");
  }
  const auto& name = request.arguments.front();
  const auto valid = characters::CharacterCreation::ValidateName(name);
  if (valid != 0) {
    result.response.result = valid == 6 ? Response::kRenameInvalidLength
                                        : Response::kRenameInvalidCharacters;
  } else if (name_taken || name == character.character_name) {
    result.response.result = Response::kRenameNameTaken;
  }
  if (result.response.result != Response::kRenameSuccess) {
    result.packets = {result.response.Encode()};
    return result;
  }
  result.character.character_name = name;
  --slot->quantity;
  result.response.arguments = {name};
  // S813 confirms the name; S802 refreshes the consumed card. The client
  // dialog asks for a relogin to apply the name. S105 is statistics and
  // is NOT assumed to rename the local user. No forced logout is inferred.
  result.packets = {
      result.response.Encode(),
      result.inventory
          .CreateNotification(character.account_id, observed_at_unix_seconds)
          .Encode()};
  return result;
}
}  // namespace bubble_fighter::server::features::inventory
