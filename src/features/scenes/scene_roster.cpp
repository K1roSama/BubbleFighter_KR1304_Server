#include "features/scenes/scene_roster.h"

#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "features/characters/character_creation.h"

namespace bubble_fighter::server::features::scenes {
namespace {
void ValidateCharacter(const characters::CharacterRecord& character) {
  if (character.account_id == 0 || character.character_id == 0 ||
      character.character_id > std::numeric_limits<std::uint32_t>::max() ||
      characters::CharacterCreation::ValidateName(character.character_name) !=
          0 ||
      !characters::CharacterCreation::SupportsSelection(
          character.character_type, character.color_index) ||
      !character.statistics.IsValid()) {
    throw std::invalid_argument("Invalid persisted scene character.");
  }
}

protocol::PartySupplementaryRecord BindEquipment(
    const characters::CharacterRecord& character,
    const protocol::PartySupplementaryRecord& equipment, std::uint32_t key) {
  // E050: 0x00817500 -> 0x00817DF0 stores Value; 0x00817E50
  // returns that value minus 300000. SupportsSelection bounds the sum.
  // The supplied collections are preserved; no starter items invented.
  auto result = equipment;
  result.key = key;
  result.value = 300000U + character.character_type;
  return result;
}
}  // namespace

protocol::GotoRoomNotification SceneRoster::Compose(
    protocol::GotoRoomNotification envelope, std::span<const SceneSlot> slots,
    std::uint32_t local_slot, std::uint64_t authenticated_account_id) {
  if (envelope.descriptor.session_type != 1 &&
      envelope.descriptor.session_type != 3) {
    throw std::invalid_argument("Only menu/plaza scene rosters are supported.");
  }
  return Bind(std::move(envelope), slots, local_slot, authenticated_account_id);
}

protocol::GotoRoomNotification SceneRoster::ComposePractice(
    protocol::GotoRoomNotification envelope, const SceneSlot& player,
    std::uint64_t authenticated_account_id) {
  if (envelope.descriptor.session_type != 2 ||
      envelope.descriptor.configuration.short_at4 != 40 ||
      envelope.descriptor.configuration.value_at0 != 0) {
    throw std::invalid_argument(
        "Only the local single-player training configuration is supported.");
  }
  // 9D9250 selects capacity {12,12,16} for type 2, mode 40. Slot zero
  // is within both participant and total bounds; no empty remote peers.
  return Bind(std::move(envelope), std::span{&player, 1}, 0,
              authenticated_account_id);
}

protocol::GotoRoomNotification SceneRoster::ComposeRoom(
    protocol::GotoRoomNotification envelope, std::span<const SceneSlot> slots,
    std::uint32_t local_slot, std::uint64_t authenticated_account_id) {
  if (envelope.descriptor.session_type != 2 ||
      envelope.descriptor.configuration.short_at4 != 1 || slots.size() > 16 ||
      envelope.descriptor.configuration.value_at0 != 0) {
    throw std::invalid_argument("Unsupported ordinary room configuration.");
  }
  return Bind(std::move(envelope), slots, local_slot, authenticated_account_id);
}

protocol::GotoRoomNotification SceneRoster::Bind(
    protocol::GotoRoomNotification envelope, std::span<const SceneSlot> slots,
    std::uint32_t local_slot, std::uint64_t authenticated_account_id) {
  if (!envelope.members.empty() || !envelope.records_at564.empty() ||
      !envelope.records_at588.empty()) {
    throw std::invalid_argument("Scene envelope already contains a roster.");
  }
  if (slots.empty() || slots.size() > kMaxSlots || local_slot >= slots.size() ||
      authenticated_account_id == 0 || !slots[local_slot].character ||
      slots[local_slot].character->account_id != authenticated_account_id) {
    throw std::invalid_argument(
        "Local scene slot does not belong to the authenticated account.");
  }

  std::unordered_set<std::uint64_t> character_ids;
  std::unordered_set<std::uint64_t> account_ids;
  envelope.members.reserve(slots.size());
  for (std::size_t index = 0; index < slots.size(); ++index) {
    const auto& slot = slots[index];
    auto member = slot.state;
    if (!slot.character) {
      if (slot.equipment || member.booleans_at0[0] != 0 ||
          member.value_at8 != 0 || !member.text_at12.empty() ||
          member.optional_at204 ||
          member.marker_at204 ==
              protocol::GotoRoomNotification::kPresentMarker) {
        throw std::invalid_argument("Empty scene slot contains player state.");
      }
    } else {
      const auto& character = *slot.character;
      ValidateCharacter(character);
      if (!slot.equipment ||
          !character_ids.insert(character.character_id).second ||
          !account_ids.insert(character.account_id).second) {
        throw std::invalid_argument(
            "Duplicate scene player or missing equipment snapshot.");
      }
      // 0x009DA240 excludes members with the optional +204 object.
      // This path handles ordinary persisted players, not that mode.
      if (member.optional_at204 ||
          member.marker_at204 ==
              protocol::GotoRoomNotification::kPresentMarker) {
        throw std::invalid_argument(
            "Special scene member identity is unsupported.");
      }
      member.booleans_at0[0] = 1;
      member.value_at8 = static_cast<std::uint32_t>(character.character_id);
      member.text_at12 = character.character_name;
      // Reader sign-extends this short to member +40. The supported
      // creation choices are positive and fit without truncation.
      member.signed_shorts_at36[1] =
          static_cast<std::uint16_t>(character.color_index);
      // E051: 0x00912A00 passes member +172 to LevelSymbolIcon.
      member.values_at172[0] = character.statistics.level;

      // E050: +564 -> 0x0081D060 uses a slot index, while +588 ->
      // 0x0081CCE0 uses the player identity. Never reuse one key in both.
      envelope.records_at564.push_back(BindEquipment(
          character, *slot.equipment, static_cast<std::uint32_t>(index)));
      envelope.records_at588.push_back(
          BindEquipment(character, *slot.equipment, member.value_at8));
    }
    // Position in the vector is the runtime slot; retain explicit empty
    // entries. Other flags, team, endpoints and opaque fields are inputs.
    envelope.members.push_back(std::move(member));
  }
  // E050: +728 -> context +580 -> 0x009E4490. +548 is NOT this index.
  envelope.value_at728 = local_slot;
  return envelope;
}

protocol::InventoryViewUpdateNotification SceneRoster::CreateInventoryView(
    std::uint64_t authenticated_account_id,
    const characters::CharacterRecord& character,
    const protocol::PartySupplementaryRecord& equipment) {
  ValidateCharacter(character);
  if (authenticated_account_id == 0 ||
      character.account_id != authenticated_account_id) {
    throw std::invalid_argument("Inventory view belongs to another account.");
  }
  protocol::InventoryViewUpdateNotification result;
  result.view = BindEquipment(
      character, equipment, static_cast<std::uint32_t>(character.character_id));
  return result;
}
}  // namespace bubble_fighter::server::features::scenes
