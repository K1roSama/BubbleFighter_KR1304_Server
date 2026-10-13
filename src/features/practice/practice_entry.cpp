#include "features/practice/practice_entry.h"

#include <stdexcept>
#include <utility>

#include "features/scenes/scene_roster.h"
#include "protocol/battle/game_load_notification.h"
#include "protocol/battle/game_transport_options_notification.h"

namespace bubble_fighter::server::features::practice {
namespace {
constexpr std::array<std::uint32_t, 13> kRules{
    3000,   6000,   4000,   11000,  45000, 142000, 20,
    240000, 241000, 240000, 240000, 15000, 15000};
constexpr std::u16string_view kMapKey = u"rel_Village07_Easy02";
constexpr std::u16string_view kMapName = u"泡泡村小河边休闲区";
}  // namespace

void PracticeEntry::Validate(
    const protocol::ClientToServerMessage0x006B& request) {
  // Do not let a client select arbitrary filesystem paths, rewards,
  // modes or rule values. See TutorialFlowResourceProfile.json (E068).
  if (request.rules != kRules || request.rule_flags != 16 ||
      request.directory != kMapKey || request.map_key != kMapKey ||
      request.map_name != kMapName || request.difficulty != 1) {
    throw std::invalid_argument("Unsupported local practice map or rules.");
  }
}

std::vector<protocol::TcpPacket> PracticeEntry::Prepare(
    const protocol::ClientToServerMessage0x006B& request,
    const characters::CharacterRecord& character,
    std::uint64_t authenticated_account_id,
    sessions::MenuInitializationData data,
    std::uint64_t observed_at_unix_seconds) {
  Validate(request);
  if (data.scene.descriptor.session_type != 1 ||
      data.equipment.key != character.character_id) {
    throw std::invalid_argument(
        "Practice entry requires a fresh trusted menu snapshot.");
  }

  // Refresh validates inventory ownership, presets and packet capacity.
  // All packets are encoded before the caller changes its session state.
  auto refresh = inventory::InventoryRefresh::Prepare(
      character, authenticated_account_id, data.inventory, data.equipment,
      data.presets, observed_at_unix_seconds);
  auto& descriptor = data.scene.descriptor;
  descriptor.session_type = 2;
  descriptor.value_at4 = 3;  // 94D7A0: Launch (not Play=4).
  auto& config = descriptor.configuration;
  config.value_at0 = 0;  // Explicit native baseline: no special session flags.
  config.short_at4 =
      40;  // Sign-extended into model +144, protected at context +1776.
  config.texts_at44[0] = kMapKey;   // model +184
  config.texts_at44[1] = kMapKey;   // model +208: directory
  config.texts_at44[2] = kMapName;  // model +232: display name
  scenes::SceneSlot player;
  player.character = character;
  player.state = std::move(data.member);
  player.equipment = std::move(data.equipment);
  auto scene = scenes::SceneRoster::ComposePractice(
      std::move(data.scene), player, authenticated_account_id);

  protocol::GameLoadNotification load;
  // 9E5EB7..9E5FF3 reorders {directory,name,key} into {key,directory,name}.
  load.texts_at0 = {std::u16string{kMapKey}, std::u16string{kMapName},
                    std::u16string{kMapKey}};
  load.value_at72 = 3;
  // Unknown flags and collections use an explicit local solo baseline.
  // Remaining unknown scalar fields retain explicit zero values. This
  // permits loading only; it does not authorize a match or grant rewards.
  std::vector<protocol::TcpPacket> packets;
  packets.reserve(refresh.size() + 3);
  packets.push_back(scene.Encode());
  for (auto& packet : refresh) {
    packets.push_back(std::move(packet));
  }
  // E082: explicitly select the connected main TCP path, without relay,
  // UDP or movement deltas, before map loading. This is local solo policy;
  // it does not imply that every gameplay message can safely be echoed.
  packets.push_back(
      protocol::GameTransportOptionsNotification{0, 0, 0}.Encode());
  packets.push_back(load.Encode());
  return packets;
}
}  // namespace bubble_fighter::server::features::practice
