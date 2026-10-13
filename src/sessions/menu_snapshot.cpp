#include "sessions/menu_snapshot.h"

#include <cstdint>
#include <limits>
#include <utility>

#include "features/characters/character_profile.h"
#include "features/characters/character_store.h"
#include "features/inventory/inventory_presentation.h"
#include "features/inventory/inventory_slot.h"
#include "sessions/menu_initialization.h"

namespace bubble_fighter::server::sessions {
MenuInitializationData CreateMenuSnapshot(
    const features::characters::CharacterRecord& character,
    features::inventory::CharacterInventory inventory,
    std::uint64_t observed_at_unix_seconds) {
  MenuInitializationData data;
  data.equipment = features::inventory::CreateInventoryEquipment(
      character, inventory, observed_at_unix_seconds);
  constexpr auto missing = std::numeric_limits<std::uint32_t>::max();
  // 0x01159720 / 0x011527E0: native FighterInfo baseline. These are
  // initial local state, not recovered original-server rewards/unlocks.
  data.profile = features::characters::CreateCharacterProfileBaseline();

  auto& descriptor = data.scene.descriptor;
  descriptor.session_type = 1;  // Local solo menu, slot zero.
  // 0x0113B7B0 and 0x0113C3E0; resource default from 0x00408000.
  descriptor.value_at68 = missing;
  descriptor.wide_at80 = std::numeric_limits<std::uint64_t>::max();
  auto& configuration = descriptor.configuration;
  configuration.short_at4 = 1;
  configuration.texts_at44[0] = u"Random";
  configuration.value_at140 = 1;
  configuration.texts_at144[0] = u"Random";
  configuration.texts_at144[2] = u"Random";
  configuration.texts_at144[4] = u"Random";
  configuration.booleans_at322[2] = 1;
  configuration.booleans_at340 = {1, 1};
  configuration.value_at404 = missing;

  // 0x01143030: native SessionMember defaults; known identity, color,
  // level and character-model item are bound by SceneRoster later.
  data.member.byte_at3 = 0xFF;
  data.member.value_at88 = missing;
  data.member.value_at108 = missing;
  data.member.values_at172[2] = 1;
  const auto player_id = static_cast<std::uint32_t>(character.character_id);
  data.presets.player_id = player_id;
  // This generic assembler grants no items or pages. The MySQL provider
  // installs its loaded primary-weapon presets separately (E066).
  data.inventory = std::move(inventory);
  return data;
}
}  // namespace bubble_fighter::server::sessions
