#include "sessions/menu_initialization.h"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "features/characters/character_profile.h"
#include "features/characters/character_store.h"
#include "features/scenes/scene_roster.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::sessions {
std::vector<protocol::TcpPacket> MenuInitialization::Prepare(
    const features::characters::CharacterRecord& character,
    std::uint64_t authenticated_account_id, MenuInitializationData data,
    std::uint64_t observed_at_unix_seconds) {
  if (data.scene.descriptor.session_type != 1 ||
      data.inventory.account_id != authenticated_account_id ||
      data.inventory.character_id != character.character_id ||
      data.presets.player_id != character.character_id ||
      data.equipment.key != character.character_id ||
      data.presets.records.size() > kMaxPresetRecords) {
    throw std::invalid_argument(
        "Menu initialization has inconsistent ownership, scene or presets.");
  }

  auto profile = features::characters::CharacterProfile::BindKnownFields(
      std::move(data.profile), character, authenticated_account_id);
  // Local menu policy: one authenticated player at slot zero. Both S117
  // maps and S806 are derived from the very same supplied equipment state.
  std::array<features::scenes::SceneSlot, 1> slots;
  slots[0].state = std::move(data.member);
  slots[0].character = character;
  slots[0].equipment = data.equipment;
  auto scene = features::scenes::SceneRoster::Compose(
      std::move(data.scene), slots, 0, authenticated_account_id);
  auto equipment = features::scenes::SceneRoster::CreateInventoryView(
      authenticated_account_id, character, data.equipment);
  auto inventory = data.inventory.CreateNotification(authenticated_account_id,
                                                     observed_at_unix_seconds);

  // S3 clears inventory; S117 establishes the shared context and view
  // indexes; S837 looks up the player's view. This is the local ordered
  // initialization policy, not a claimed original packet capture.
  std::vector<protocol::TcpPacket> packets;
  packets.reserve(5);
  packets.push_back(profile.Encode());
  packets.push_back(scene.Encode());
  packets.push_back(equipment.Encode());
  packets.push_back(data.presets.Encode());
  packets.push_back(inventory.Encode());
  if (data.cash) {
    if (data.cash->character_id != character.character_id) {
      throw std::invalid_argument("Menu cash owner mismatch.");
    }
    packets.push_back(
        data.cash->CreateNotification(authenticated_account_id).Encode());
  }
  return packets;
}
}  // namespace bubble_fighter::server::sessions
