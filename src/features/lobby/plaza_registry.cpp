#include "features/lobby/plaza_registry.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

#include "features/chat/scene_chat.h"
#include "protocol/rooms/room_member_left_notification.h"
#include "protocol/rooms/room_slot_update_notification.h"
#include "protocol/scenes/scene_descriptor_notification.h"

namespace bubble_fighter::server::features::lobby {
PlazaLease::PlazaLease(std::shared_ptr<PlazaRegistry> registry,
                       std::uint16_t scene, std::uint64_t account) noexcept
    : registry_(std::move(registry)), scene_(scene), account_(account) {}
PlazaLease::PlazaLease(PlazaLease&& other) noexcept
    : registry_(std::move(other.registry_)),
      scene_(std::exchange(other.scene_, std::uint16_t{})),
      account_(std::exchange(other.account_, 0)) {}
PlazaLease& PlazaLease::operator=(PlazaLease&& other) noexcept {
  if (this != &other) {
    Release();
    registry_ = std::move(other.registry_);
    scene_ = std::exchange(other.scene_, std::uint16_t{});
    account_ = std::exchange(other.account_, 0);
  }
  return *this;
}
PlazaLease::~PlazaLease() { Release(); }
PlazaLease::operator bool() const noexcept { return registry_ != nullptr; }
void PlazaLease::Release() noexcept {
  if (registry_) {
    auto registry = std::move(registry_);
    registry->Leave(scene_, account_);
  }
}
void PlazaLease::Move(const protocol::PlazaMovementRequest& movement) {
  if (registry_) {
    registry_->Move(scene_, account_, movement);
  }
}
void PlazaLease::Refresh(scenes::SceneSlot player, bool send_snapshot) {
  if (registry_) {
    registry_->Refresh(scene_, account_, std::move(player), send_snapshot);
  }
}
void PlazaLease::Chat(const features::chat::SceneChat::Message& request) {
  if (registry_) {
    registry_->Chat(scene_, account_, request);
  }
}

void PlazaRegistry::Chat(std::uint16_t scene_id, std::uint64_t account,
                         const features::chat::SceneChat::Message& request) {
  std::lock_guard lock(mutex_);
  const auto& scene = scenes_.at(scene_id);
  const auto& sender = scene.members.at(account);
  if (request.channel != 0) {
    Deliver(
        sender,
        {features::chat::SceneChat::Notice(u"队伍聊天仅在房间或对战中可用。")});
    return;
  }
  const auto packet = features::chat::SceneChat::Create(
                          static_cast<std::int16_t>(sender.slot),
                          sender.player.character->character_name, request)
                          .Encode();
  for (const auto& [id, peer] : scene.members) {
    (void)id;
    Deliver(peer, {packet});
  }
}

protocol::GotoRoomNotification PlazaRegistry::Snapshot(const Scene& scene,
                                                       std::uint64_t account) {
  auto envelope = scene.envelope;
  std::vector<scenes::SceneSlot> slots;
  auto host = static_cast<std::uint32_t>(kCapacity);
  for (const auto& [id, member] : scene.members) {
    (void)id;
    slots.resize(
        std::max(slots.size(), static_cast<std::size_t>(member.slot + 1)));
    slots[member.slot] = member.player;
    host = std::min(host, member.slot);
  }
  envelope.descriptor.session_type = 3;
  envelope.descriptor.shorts_at32 = {
      static_cast<std::uint16_t>(scene.members.size()), 0,
      static_cast<std::uint16_t>(kCapacity)};
  envelope.value_at544 = host;
  return scenes::SceneRoster::Compose(std::move(envelope), slots,
                                      scene.members.at(account).slot, account);
}
protocol::TcpPacket PlazaRegistry::Update(
    const protocol::GotoRoomNotification& snapshot) {
  const auto slot = snapshot.value_at728;
  protocol::RoomSlotUpdateNotification m;
  m.slot = slot;
  m.member = snapshot.members.at(slot);
  m.team = snapshot.team;
  for (const auto& r : snapshot.records_at564) {
    if (r.key == slot) {
      m.records_at564.push_back(r);
    }
  }
  for (const auto& r : snapshot.records_at588) {
    if (r.key == m.member.value_at8) {
      m.records_at588.push_back(r);
    }
  }
  return m.Encode();
}
void PlazaRegistry::Deliver(const Member& member,
                            std::vector<protocol::TcpPacket> packets) noexcept {
  try {
    member.sink(std::move(packets));
  } catch (...) {
  }
}

PlazaLease PlazaRegistry::Enter(protocol::GotoRoomNotification envelope,
                                scenes::SceneSlot player, std::uint64_t account,
                                rooms::RoomPacketSink sink) {
  auto self = shared_from_this();
  if (!account || !player.character ||
      player.character->account_id != account || !player.equipment ||
      player.equipment->key != player.character->character_id || !sink) {
    throw std::invalid_argument("Invalid authenticated plaza member.");
  }
  std::lock_guard lock(mutex_);
  std::uint16_t chosen = 0;
  for (const auto& [id, scene] : scenes_) {
    for (const auto& [other, member] : scene.members) {
      if (other == account || member.player.character->character_id ==
                                  player.character->character_id) {
        throw std::invalid_argument("Identity already in a plaza.");
      }
    }
    if (!chosen && scene.members.size() < kCapacity) {
      chosen = id;
    }
  }
  if (!chosen) {
    for (std::uint16_t id = 1; id <= 512; ++id) {
      if (!scenes_.contains(id)) {
        chosen = id;
        break;
      }
    }
    if (!chosen) {
      throw std::runtime_error("Local plaza registry full.");
    }
  }
  const auto existing = scenes_.find(chosen);
  Scene candidate = existing == scenes_.end() ? Scene{std::move(envelope), {}}
                                              : existing->second;
  candidate.envelope.descriptor.short_at0 = chosen;
  std::array<bool, kCapacity> used{};
  for (const auto& [id, member] : candidate.members) {
    (void)id;
    used.at(member.slot) = true;
  }
  const auto seat = static_cast<std::uint32_t>(
      std::find(used.begin(), used.end(), false) - used.begin());
  candidate.members.emplace(
      account, Member{seat, std::move(player), std::move(sink), {}});
  const auto snapshot = Snapshot(candidate, account);
  const auto update = Update(snapshot);
  const auto descriptor =
      protocol::SceneDescriptorNotification{snapshot.descriptor}.Encode();
  std::vector<protocol::TcpPacket> own{snapshot.Encode()};
  for (const auto& [id, member] : candidate.members) {
    if (id != account && member.movement) {
      own.push_back(protocol::PlazaMovementNotification{
          static_cast<std::uint32_t>(member.player.character->character_id),
          *member.movement}
                        .Encode());
    }
  }
  auto& committed =
      scenes_.insert_or_assign(chosen, std::move(candidate)).first->second;
  for (const auto& [id, member] : committed.members) {
    if (id != account) {
      Deliver(member, {update, descriptor});
    }
  }
  Deliver(committed.members.at(account), std::move(own));
  return PlazaLease(std::move(self), chosen, account);
}
void PlazaRegistry::Leave(std::uint16_t scene_id,
                          std::uint64_t account) noexcept {
  std::lock_guard lock(mutex_);
  auto found = scenes_.find(scene_id);
  if (found == scenes_.end()) {
    return;
  }
  auto& scene = found->second;
  const auto member = scene.members.find(account);
  if (member == scene.members.end()) {
    return;
  }
  const auto slot = member->second.slot;
  scene.members.erase(member);
  if (scene.members.empty()) {
    scenes_.erase(found);
    return;
  }
  try {
    protocol::RoomMemberLeftNotification left;
    left.slot = slot;
    left.team = scene.envelope.team;
    const auto packet = left.Encode();
    const auto snapshot = Snapshot(scene, scene.members.begin()->first);
    const auto descriptor =
        protocol::SceneDescriptorNotification{snapshot.descriptor}.Encode();
    for (const auto& [id, peer] : scene.members) {
      (void)id;
      Deliver(peer, {packet, descriptor});
    }
  } catch (...) {
  }  // The lease still vacates the seat on a failed closing connection.
}
void PlazaRegistry::Move(std::uint16_t scene_id, std::uint64_t account,
                         const protocol::PlazaMovementRequest& movement) {
  movement.Validate();
  std::lock_guard lock(mutex_);
  auto& scene = scenes_.at(scene_id);
  auto& member = scene.members.at(account);
  const auto packet =
      protocol::PlazaMovementNotification{
          static_cast<std::uint32_t>(member.player.character->character_id),
          movement}
          .Encode();
  member.movement = movement;
  for (const auto& [id, peer] : scene.members) {
    if (id != account) {
      Deliver(peer, {packet});
    }
  }
}
void PlazaRegistry::Refresh(std::uint16_t scene_id, std::uint64_t account,
                            scenes::SceneSlot player, bool send_snapshot) {
  std::lock_guard lock(mutex_);
  auto& scene = scenes_.at(scene_id);
  auto candidate = scene;
  auto& member = candidate.members.at(account);
  if (!player.character || player.character->account_id != account ||
      !player.equipment ||
      player.equipment->key != player.character->character_id ||
      player.character->character_id != member.player.character->character_id) {
    throw std::invalid_argument("Plaza refresh identity mismatch.");
  }
  member.player = std::move(player);
  const auto snapshot = Snapshot(candidate, account);
  const auto update = Update(snapshot);
  std::vector<protocol::TcpPacket> own;
  if (send_snapshot) {
    member.movement.reset();
    own.push_back(snapshot.Encode());
    for (const auto& [id, peer] : candidate.members) {
      if (id != account && peer.movement) {
        own.push_back(protocol::PlazaMovementNotification{
            static_cast<std::uint32_t>(peer.player.character->character_id),
            *peer.movement}
                          .Encode());
      }
    }
  }
  scene = std::move(candidate);
  for (const auto& [id, peer] : scene.members) {
    if (id != account) {
      Deliver(peer, {update});
    }
  }
  if (send_snapshot) {
    Deliver(scene.members.at(account), std::move(own));
  }
}
}  // namespace bubble_fighter::server::features::lobby
