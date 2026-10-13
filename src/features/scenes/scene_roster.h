#ifndef BUBBLEFIGHTER_SERVER_FEATURES_SCENES_SCENE_ROSTER_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_SCENES_SCENE_ROSTER_H_

#include <cstddef>
#include <optional>
#include <span>

#include "features/characters/character_store.h"
#include "protocol/inventory/inventory_view_update_notification.h"
#include "protocol/scenes/goto_room_notification.h"

namespace bubble_fighter::server::features::scenes {
// The caller supplies the remaining scene state and a persisted equipment
// snapshot. These are required inputs, not constructor-default game data.
struct SceneSlot {
  protocol::SessionMember state;
  std::optional<characters::CharacterRecord> character;
  std::optional<protocol::PartySupplementaryRecord> equipment;
};

class SceneRoster {
 public:
  // E050: menu/plaza models have 36 addressable slots. Room capacity also
  // depends on game mode; room snapshots are deliberately not accepted yet.
  static constexpr std::size_t kMaxSlots = 36;

  // Populates an unbound menu/plaza envelope. No input is modified, even
  // on failure. This does not authorize sending or initialize game state.
  static protocol::GotoRoomNotification Compose(
      protocol::GotoRoomNotification envelope, std::span<const SceneSlot> slots,
      std::uint32_t local_slot, std::uint64_t authenticated_account_id);

  // E070: narrowly scoped single-player mode 40. This does not admit
  // ordinary room creation, spectators or arbitrary mode capacities.
  static protocol::GotoRoomNotification ComposePractice(
      protocol::GotoRoomNotification envelope, const SceneSlot& player,
      std::uint64_t authenticated_account_id);

  // Ordinary mode-1 room only; shares authenticated identity binding.
  static protocol::GotoRoomNotification ComposeRoom(
      protocol::GotoRoomNotification envelope, std::span<const SceneSlot> slots,
      std::uint32_t local_slot, std::uint64_t authenticated_account_id);

  // Same authoritative identity/character mapping as both S117 maps.
  static protocol::InventoryViewUpdateNotification CreateInventoryView(
      std::uint64_t authenticated_account_id,
      const characters::CharacterRecord& character,
      const protocol::PartySupplementaryRecord& equipment);

 private:
  static protocol::GotoRoomNotification Bind(
      protocol::GotoRoomNotification envelope, std::span<const SceneSlot> slots,
      std::uint32_t local_slot, std::uint64_t authenticated_account_id);
};
}  // namespace bubble_fighter::server::features::scenes

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_SCENES_SCENE_ROSTER_H_
