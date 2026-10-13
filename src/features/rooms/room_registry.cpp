#include "features/rooms/room_registry.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "features/chat/scene_chat.h"
#include "features/practice/practice_session.h"
#include "features/rooms/room_roster.h"
#include "protocol/battle/game_return_to_room_notification.h"
#include "protocol/battle/game_transport_options_notification.h"
#include "protocol/rooms/room_host_changed_notification.h"

namespace bubble_fighter::server::features::rooms {
const RoomOccupant* RoomRecord::FindSlot(std::uint32_t slot) const noexcept {
  const auto occupant = std::find_if(
      occupants.begin(), occupants.end(),
      [&](const RoomOccupant& entry) { return entry.slot == slot; });
  return occupant == occupants.end() ? nullptr : &*occupant;
}

RoomLease::RoomLease(std::shared_ptr<RoomRegistry> registry,
                     std::uint16_t room_id, std::uint64_t account_id) noexcept
    : registry_(std::move(registry)),
      room_id_(room_id),
      account_id_(account_id) {}

RoomLease::RoomLease(RoomLease&& other) noexcept
    : registry_(std::move(other.registry_)),
      room_id_(std::exchange(other.room_id_, std::uint16_t{})),
      account_id_(std::exchange(other.account_id_, std::uint64_t{})) {}

RoomLease& RoomLease::operator=(RoomLease&& other) noexcept {
  if (this != &other) {
    Release();
    registry_ = std::move(other.registry_);
    room_id_ = std::exchange(other.room_id_, std::uint16_t{});
    account_id_ = std::exchange(other.account_id_, std::uint64_t{});
  }
  return *this;
}

RoomLease::~RoomLease() { Release(); }

std::uint16_t RoomLease::RoomId() const noexcept {
  return registry_ ? room_id_ : 0;
}
RoomLease::operator bool() const noexcept { return registry_ != nullptr; }

void RoomLease::Release() noexcept {
  if (registry_) {
    registry_->Leave(room_id_, account_id_);
  }
  registry_.reset();
  room_id_ = 0;
  account_id_ = 0;
}

bool RoomLease::SetMemberState(const std::array<std::uint32_t, 3>& state) {
  if (!registry_) {
    throw std::logic_error("Room state change without a room.");
  }
  return registry_->SetMemberState(room_id_, account_id_, state);
}

RoomStartStatus RoomLease::Start(const protocol::TcpPacket& packet,
                                 std::size_t minimum_players,
                                 std::uint32_t duration_milliseconds,
                                 std::u16string_view trusted_map_key) {
  if (!registry_) {
    throw std::logic_error("Game start without a room.");
  }
  return registry_->Start(room_id_, account_id_, packet, minimum_players,
                          duration_milliseconds, trusted_map_key);
}

void RoomLease::ProcessGame(const protocol::TcpPacket& packet) {
  if (!registry_) {
    throw std::logic_error("Game message outside a room.");
  }
  registry_->ProcessGame(room_id_, account_id_, packet);
}
void RoomLease::Poll() {
  if (registry_) {
    registry_->Poll(room_id_, account_id_);
  }
}
void RoomLease::Chat(const features::chat::SceneChat::Message& request) {
  if (registry_) {
    registry_->Chat(room_id_, account_id_, request);
  }
}
void RoomLease::RefreshCharacter(const characters::CharacterRecord& character) {
  if (registry_) {
    registry_->RefreshCharacter(room_id_, account_id_, character);
  }
}
bool RoomLease::ChangeInventory(
    const std::function<RoomInventoryChange()>& change) {
  return registry_ && registry_->ChangeInventory(room_id_, account_id_, change);
}
bool RoomRegistry::ChangeInventory(
    std::uint16_t room_id, std::uint64_t account_id,
    const std::function<RoomInventoryChange()>& change) {
  {
    std::lock_guard lock(mutex_);
    auto& room = rooms_.at(room_id);
    if (!FindMemberLocked(room, account_id)) {
      throw std::logic_error("Inventory change by non-member.");
    }
    if (room.record.state != RoomState::kWaiting ||
        !room.inventory_changes.insert(account_id).second) {
      return false;
    }
  }
  // Reserve the waiting state, then release the registry lock for database
  // work. Start cannot bind stale equipment while the change is in flight.
  RoomInventoryChange result;
  try {
    result = change();
  } catch (...) {
    std::lock_guard lock(mutex_);
    const auto found = rooms_.find(room_id);
    if (found != rooms_.end()) {
      found->second.inventory_changes.erase(account_id);
    }
    throw;
  }
  std::lock_guard lock(mutex_);
  auto& room = rooms_.at(room_id);
  room.inventory_changes.erase(account_id);
  auto* member = FindMemberLocked(room, account_id);
  if (!member || result.character.account_id != account_id ||
      result.character.character_id != member->character.character_id ||
      result.equipment.key != result.character.character_id ||
      !result.character.statistics.IsValid()) {
    throw std::invalid_argument("Room inventory identity mismatch.");
  }
  if (member->character == result.character &&
      member->equipment == result.equipment) {
    return true;
  }
  const auto previous = *member;
  member->character = std::move(result.character);
  member->equipment = std::move(result.equipment);
  member->member
      .booleans_at96 = {};  // Changed loadout requires preparing again.
  protocol::TcpPacket packet;
  try {
    packet = RoomRoster::SlotUpdate(room.record, member->slot).Encode();
  } catch (...) {
    *member = previous;
    throw;
  }
  Broadcast(room, {packet});
  return true;
}
void RoomRegistry::RefreshCharacter(
    std::uint16_t room_id, std::uint64_t account_id,
    const characters::CharacterRecord& character) {
  std::lock_guard lock(mutex_);
  auto& room = rooms_.at(room_id);
  auto* member = FindMemberLocked(room, account_id);
  if (!member || character.account_id != account_id ||
      character.character_id != member->character.character_id ||
      character.character_name != member->character.character_name ||
      character.character_type != member->character.character_type ||
      character.color_index != member->character.color_index ||
      !character.statistics.IsValid()) {
    throw std::invalid_argument("Room commerce identity mismatch.");
  }
  // Shop transactions do not change the equipped loadout or room seat.
  // Keep the owned snapshot in sync so Poll cannot restore old balances.
  member->character.statistics = character.statistics;
}
void RoomRegistry::Chat(std::uint16_t room_id, std::uint64_t account_id,
                        const features::chat::SceneChat::Message& request) {
  std::lock_guard lock(mutex_);
  const auto& room = rooms_.at(room_id);
  const auto sender = std::find_if(
      room.record.occupants.begin(), room.record.occupants.end(),
      [&](const auto& member) { return member.account_id == account_id; });
  if (sender == room.record.occupants.end()) {
    throw std::logic_error("Chat from non-member.");
  }
  const auto packet = features::chat::SceneChat::Create(
                          static_cast<std::int16_t>(sender->slot),
                          sender->character.character_name, request)
                          .Encode();
  for (const auto& peer : room.record.occupants) {
    if (request.channel == 0 || peer.slot == sender->slot ||
        ((sender->member.signed_byte_at4 == 1 ||
          sender->member.signed_byte_at4 == 2) &&
         peer.member.signed_byte_at4 == sender->member.signed_byte_at4)) {
      Deliver(room.sinks.at(peer.slot), {packet});
    }
  }
}
std::optional<characters::CharacterRecord> RoomLease::Character() const {
  if (!registry_) {
    return {};
  }
  const auto room = registry_->Find(room_id_);
  if (!room) {
    return {};
  }
  for (const auto& occupant : room->occupants) {
    if (occupant.account_id == account_id_) {
      return occupant.character;
    }
  }
  return {};
}
void RoomLease::ReturnToWaiting() {
  if (!registry_) {
    throw std::logic_error("Game exit outside a room.");
  }
  registry_->ReturnToWaiting(room_id_, account_id_);
}

RoomReservation::RoomReservation(std::shared_ptr<RoomRegistry> registry,
                                 std::uint16_t room_id,
                                 std::uint64_t account_id,
                                 std::uint64_t character_id) noexcept
    : registry_(std::move(registry)),
      room_id_(room_id),
      account_id_(account_id),
      character_id_(character_id) {}

RoomReservation::RoomReservation(RoomReservation&& other) noexcept
    : registry_(std::move(other.registry_)),
      room_id_(std::exchange(other.room_id_, std::uint16_t{})),
      account_id_(std::exchange(other.account_id_, std::uint64_t{})),
      character_id_(std::exchange(other.character_id_, std::uint64_t{})) {}

RoomReservation& RoomReservation::operator=(RoomReservation&& other) noexcept {
  if (this != &other) {
    Abandon();
    registry_ = std::move(other.registry_);
    room_id_ = std::exchange(other.room_id_, std::uint16_t{});
    account_id_ = std::exchange(other.account_id_, std::uint64_t{});
    character_id_ = std::exchange(other.character_id_, std::uint64_t{});
  }
  return *this;
}

RoomReservation::~RoomReservation() { Abandon(); }

std::uint16_t RoomReservation::RoomId() const noexcept { return room_id_; }

RoomLease RoomReservation::Commit(
    RoomRecord record, RoomPacketSink host_sink,
    std::vector<protocol::TcpPacket> host_packets) && {
  if (!registry_) {
    throw std::logic_error("Room reservation was already used.");
  }
  registry_->Publish(room_id_, account_id_, character_id_, std::move(record),
                     std::move(host_sink), std::move(host_packets));
  // Publishing succeeded; the identity now belongs to the room and the
  // host's place in it to the lease.
  RoomLease lease(std::move(registry_), room_id_, account_id_);
  room_id_ = 0;
  return lease;
}

void RoomReservation::Abandon() noexcept {
  if (registry_) {
    registry_->Abandon(room_id_);
  }
  registry_.reset();
}

std::shared_ptr<RoomRegistry> RoomRegistry::Create(std::size_t max_rooms,
                                                   GameClock clock,
                                                   SeedFactory seed,
                                                   RoomMatchSettler settler,
                                                   MatchRewardPolicy rules) {
  if (max_rooms == 0 || max_rooms > kMaxRoomId) {
    throw std::invalid_argument("Unsupported room registry capacity.");
  }
  if (!clock) {
    clock = RoomGameSession::Clock::now;
  }
  if (!seed) {
    seed = practice::PracticeSession::CreateRandomSeed;
  }
  rules.Validate();
  return std::shared_ptr<RoomRegistry>(new RoomRegistry(
      max_rooms, std::move(clock), std::move(seed), std::move(settler), rules));
}

RoomRegistry::RoomRegistry(std::size_t max_rooms, GameClock clock,
                           SeedFactory seed, RoomMatchSettler settler,
                           MatchRewardPolicy rules)
    : max_rooms_(max_rooms),
      clock_(std::move(clock)),
      seed_(std::move(seed)),
      settlements_(
          settler ? std::make_shared<RoomSettlementQueue>(std::move(settler))
                  : nullptr),
      rules_(rules) {}

bool RoomRegistry::IsUsedLocked(std::uint16_t room_id) const {
  return rooms_.contains(room_id) || pending_.contains(room_id);
}

RoomOccupant* RoomRegistry::FindMemberLocked(Room& room,
                                             std::uint64_t account_id) {
  const auto occupant =
      std::find_if(room.record.occupants.begin(), room.record.occupants.end(),
                   [&](const RoomOccupant& entry) {
                     return entry.account_id == account_id;
                   });
  return occupant == room.record.occupants.end() ? nullptr : &*occupant;
}

void RoomRegistry::Deliver(const RoomPacketSink& sink,
                           std::vector<protocol::TcpPacket> packets) noexcept {
  if (!sink || packets.empty()) {
    return;
  }
  try {
    sink(std::move(packets));
  } catch (...) {
  }
}

void RoomRegistry::Broadcast(
    const Room& room, const std::vector<protocol::TcpPacket>& packets,
    std::optional<std::uint32_t> except_slot) noexcept {
  for (const auto& [slot, sink] : room.sinks) {
    if (slot == except_slot) {
      continue;
    }
    try {
      Deliver(sink, packets);
    } catch (...) {
    }
  }
}

RoomReserveResult RoomRegistry::Reserve(std::uint64_t account_id,
                                        std::uint64_t character_id) {
  if (account_id == 0 || character_id == 0) {
    throw std::invalid_argument("Room owner identity is missing.");
  }
  std::lock_guard lock(mutex_);
  RoomReserveResult result;
  if (by_account_.contains(account_id) ||
      by_character_.contains(character_id)) {
    result.status = RoomReserveStatus::kAlreadyInRoom;
    return result;
  }
  if (rooms_.size() + pending_.size() >= max_rooms_) {
    result.status = RoomReserveStatus::kRegistryFull;
    return result;
  }
  // Local policy: rotate through identities so a just-closed room number
  // is not immediately handed to a different room.
  auto room_id = next_room_id_;
  while (IsUsedLocked(room_id)) {
    room_id =
        room_id == kMaxRoomId ? 1 : static_cast<std::uint16_t>(room_id + 1);
  }
  pending_.emplace(room_id, Owner{account_id, character_id});
  try {
    by_account_.emplace(account_id, room_id);
    by_character_.emplace(character_id, room_id);
  } catch (...) {
    pending_.erase(room_id);
    by_account_.erase(account_id);
    by_character_.erase(character_id);
    throw;
  }
  next_room_id_ =
      room_id == kMaxRoomId ? 1 : static_cast<std::uint16_t>(room_id + 1);
  result.status = RoomReserveStatus::kReserved;
  result.reservation.emplace(
      RoomReservation(shared_from_this(), room_id, account_id, character_id));
  return result;
}

void RoomRegistry::Publish(std::uint16_t room_id, std::uint64_t account_id,
                           std::uint64_t character_id, RoomRecord record,
                           RoomPacketSink host_sink,
                           std::vector<protocol::TcpPacket> host_packets) {
  std::lock_guard lock(mutex_);
  const auto pending = pending_.find(room_id);
  if (pending == pending_.end() || pending->second.account_id != account_id ||
      pending->second.character_id != character_id) {
    throw std::logic_error("Room publication does not match its reservation.");
  }
  if (record.room_id != room_id || record.slot_count == 0 ||
      record.host_slot >= record.slot_count ||
      record.state != RoomState::kWaiting || record.occupants.size() != 1 ||
      record.occupants[0].slot != record.host_slot ||
      record.occupants[0].account_id != account_id ||
      record.occupants[0].character.character_id != character_id ||
      !host_sink) {
    throw std::invalid_argument(
        "Room record does not describe the reserved host.");
  }
  const auto host_slot = record.host_slot;
  Room room;
  room.record = std::move(record);
  room.sinks.emplace(host_slot, std::move(host_sink));
  const auto inserted = rooms_.emplace(room_id, std::move(room)).first;
  pending_.erase(pending);
  Deliver(inserted->second.sinks.at(host_slot), std::move(host_packets));
}

void RoomRegistry::Abandon(std::uint16_t room_id) noexcept {
  std::lock_guard lock(mutex_);
  const auto pending = pending_.find(room_id);
  if (pending == pending_.end()) {
    return;
  }
  by_account_.erase(pending->second.account_id);
  by_character_.erase(pending->second.character_id);
  pending_.erase(pending);
}

RoomJoinResult RoomRegistry::Join(
    std::uint16_t room_id, const std::u16string& password, RoomOccupant joiner,
    RoomPacketSink sink, std::vector<protocol::TcpPacket> joiner_prefix) {
  if (joiner.account_id == 0 ||
      joiner.character.account_id != joiner.account_id ||
      joiner.character.character_id == 0 ||
      joiner.equipment.key != joiner.character.character_id || !sink) {
    throw std::invalid_argument(
        "Room joiner does not belong to the authenticated account.");
  }
  std::lock_guard lock(mutex_);
  if (by_account_.contains(joiner.account_id) ||
      by_character_.contains(joiner.character.character_id)) {
    throw std::logic_error("Room joiner is already in a room.");
  }
  RoomJoinResult result;
  const auto entry = rooms_.find(room_id);
  if (entry == rooms_.end()) {
    return result;
  }
  auto& room = entry->second;
  auto& record = room.record;
  if (record.state != RoomState::kWaiting) {
    result.status = RoomJoinStatus::kPlaying;
    return result;
  }
  if (password != record.password) {
    result.status = RoomJoinStatus::kWrongPassword;
    return result;
  }
  std::uint32_t slot = 0;
  while (slot < record.slot_count && record.FindSlot(slot)) {
    ++slot;
  }
  if (slot == record.slot_count) {
    result.status = RoomJoinStatus::kFull;
    return result;
  }

  joiner.slot = slot;
  const auto first_team =
      std::count_if(record.occupants.begin(), record.occupants.end(),
                    [](const RoomOccupant& occupant) {
                      return occupant.member.signed_byte_at4 == 1;
                    });
  const auto second_team =
      std::count_if(record.occupants.begin(), record.occupants.end(),
                    [](const RoomOccupant& occupant) {
                      return occupant.member.signed_byte_at4 == 2;
                    });
  joiner.member.signed_byte_at4 =
      first_team <= second_team ? std::uint8_t{1} : std::uint8_t{2};
  const auto account_id = joiner.account_id;
  const auto character_id = joiner.character.character_id;
  const auto position = std::find_if(
      record.occupants.begin(), record.occupants.end(),
      [&](const RoomOccupant& occupant) { return occupant.slot > slot; });
  record.occupants.insert(position, std::move(joiner));
  std::vector<protocol::TcpPacket> others;
  std::shared_ptr<RoomRegistry> self;
  try {
    // Everything that can fail happens before anyone observes the new
    // member. No RoomLease may be destroyed here: its release would
    // re-enter this lock.
    joiner_prefix.push_back(RoomRoster::Snapshot(record, slot).Encode());
    others.push_back(RoomRoster::SlotUpdate(record, slot).Encode());
    self = shared_from_this();
    by_account_.emplace(account_id, room_id);
    by_character_.emplace(character_id, room_id);
    room.sinks.emplace(slot, std::move(sink));
  } catch (...) {
    std::erase_if(record.occupants, [&](const RoomOccupant& occupant) {
      return occupant.slot == slot;
    });
    room.sinks.erase(slot);
    by_account_.erase(account_id);
    by_character_.erase(character_id);
    throw;
  }
  Broadcast(room, others, slot);
  Deliver(room.sinks.at(slot), std::move(joiner_prefix));
  result.status = RoomJoinStatus::kJoined;
  result.room = RoomLease(std::move(self), room_id, account_id);
  return result;
}

void RoomRegistry::Leave(std::uint16_t room_id,
                         std::uint64_t account_id) noexcept {
  std::lock_guard lock(mutex_);
  const auto entry = rooms_.find(room_id);
  if (entry == rooms_.end()) {
    return;
  }
  auto& room = entry->second;
  auto& record = room.record;
  const auto* member = FindMemberLocked(room, account_id);
  if (!member) {
    return;
  }
  const auto slot = member->slot;
  by_account_.erase(account_id);
  by_character_.erase(member->character.character_id);
  std::erase_if(record.occupants, [&](const RoomOccupant& occupant) {
    return occupant.slot == slot;
  });
  room.sinks.erase(slot);
  if (record.occupants.empty()) {
    rooms_.erase(entry);
    return;
  }
  std::vector<protocol::TcpPacket> packets;
  try {
    packets.push_back(RoomRoster::MemberLeft(record, slot).Encode());
    if (record.host_slot == slot) {
      // Local policy: the lowest remaining slot becomes host. S502
      // clears that slot's ready byte (0x009DA3C0); mirror it.
      auto& host = record.occupants.front();
      record.host_slot = host.slot;
      host.member.booleans_at96[0] = 0;
      packets.push_back(
          protocol::RoomHostChangedNotification{host.slot}.Encode());
    }
  } catch (...) {
    // Out of memory while encoding: members keep a stale view, but the
    // departed member is gone from the server state either way.
    packets.clear();
  }
  Broadcast(room, packets);
  if (room.game) {
    // Local pre-match integration policy: departures cancel the shared
    // match, never strand the remaining players at a loading barrier.
    try {
      ResetGameLocked(room);
    } catch (...) {
      room.game.reset();
      record.state = RoomState::kWaiting;
    }
  }
}

bool RoomRegistry::SetMemberState(std::uint16_t room_id,
                                  std::uint64_t account_id,
                                  const std::array<std::uint32_t, 3>& state) {
  std::lock_guard lock(mutex_);
  const auto entry = rooms_.find(room_id);
  if (entry == rooms_.end()) {
    throw std::logic_error("Room state change for a closed room.");
  }
  auto& room = entry->second;
  auto* member = FindMemberLocked(room, account_id);
  if (!member) {
    throw std::logic_error("Room state change by a non-member.");
  }
  if (room.record.state != RoomState::kWaiting) {
    return false;
  }
  const auto previous = member->member.booleans_at96;
  member->member.booleans_at96 = state;
  std::vector<protocol::TcpPacket> packets;
  try {
    packets.push_back(
        RoomRoster::SlotUpdate(room.record, member->slot).Encode());
  } catch (...) {
    member->member.booleans_at96 = previous;
    throw;
  }
  Broadcast(room, packets);
  return true;
}

RoomStartStatus RoomRegistry::Start(std::uint16_t room_id,
                                    std::uint64_t account_id,
                                    const protocol::TcpPacket& packet,
                                    std::size_t minimum_players,
                                    std::uint32_t duration_milliseconds,
                                    std::u16string_view trusted_map_key) {
  std::lock_guard lock(mutex_);
  const auto entry = rooms_.find(room_id);
  if (entry == rooms_.end()) {
    throw std::logic_error("Game start for a closed room.");
  }
  auto& room = entry->second;
  auto& record = room.record;
  const auto* member = FindMemberLocked(room, account_id);
  if (!member) {
    throw std::logic_error("Game start by a non-member.");
  }
  if (member->slot != record.host_slot) {
    return RoomStartStatus::kNotHost;
  }
  if (record.state != RoomState::kWaiting) {
    return RoomStartStatus::kNotWaiting;
  }
  if (!room.inventory_changes.empty()) {
    return RoomStartStatus::kMembersNotReady;
  }
  if (record.occupants.size() < minimum_players) {
    return RoomStartStatus::kNotEnoughPlayers;
  }
  // 0x0096DD20 checks the same before sending: every occupied slot
  // other than the host has its ready byte (+96) set.
  if (std::any_of(record.occupants.begin(), record.occupants.end(),
                  [&](const RoomOccupant& occupant) {
                    return occupant.slot != record.host_slot &&
                           occupant.member.booleans_at96[0] == 0;
                  })) {
    return RoomStartStatus::kMembersNotReady;
  }
  std::vector<std::uint32_t> slots;
  for (const auto& occupant : record.occupants) {
    slots.push_back(occupant.slot);
  }
  RoomGameSession game(slots, clock_(), seed_(), duration_milliseconds);
  game.BindMap(trusted_map_key);
  for (const auto& occupant : record.occupants) {
    game.BindEquipment(occupant.slot, occupant.equipment.groups[0]);
    game.BindCharacter(occupant.slot, occupant.character.character_type);
    game.BindTeam(occupant.slot,
                  static_cast<std::uint8_t>(occupant.member.signed_byte_at4));
  }
  std::vector<protocol::TcpPacket> packets{
      protocol::GameTransportOptionsNotification{0, 0, 0}.Encode(), packet};
  const auto match_id =
      settlements_ ? CreateRoomMatchId() : std::array<std::uint8_t, 16>{};
  if (settlements_) {
    game.RequireSettlement();
  }
  room.game.emplace(std::move(game));
  room.finished_game.reset();
  room.match_id = match_id;
  room.duration_milliseconds = duration_milliseconds;
  room.pending_settlement.reset();
  room.settlement_complete = room.settlement_delayed = false;
  room.retry_settlement_at = {};
  record.state = RoomState::kLoading;
  Broadcast(room, packets);
  return RoomStartStatus::kStarted;
}

void RoomRegistry::DeliverGameLocked(
    const Room& room, const std::vector<RoomGameDelivery>& deliveries) {
  for (const auto& delivery : deliveries) {
    if (delivery.only_slot) {
      Deliver(room.sinks.at(*delivery.only_slot), {delivery.packet});
    } else {
      Broadcast(room, {delivery.packet}, delivery.except_slot);
    }
  }
}

void RoomRegistry::ResetGameLocked(Room& room, bool return_notified) {
  auto waiting = room.record;
  waiting.state = RoomState::kWaiting;
  for (auto& occupant : waiting.occupants) {
    occupant.member.booleans_at96 = {};
  }
  // S620 must reach the old context before S117 replaces it. Normal
  // result presentation already delivered it; cancellation and loading
  // timeout need the same cleanup (RoomExit: 9EA690 / 9E1D70).
  std::vector<std::pair<std::uint32_t, std::vector<protocol::TcpPacket>>>
      packets;
  for (const auto& occupant : waiting.occupants) {
    std::vector<protocol::TcpPacket> batch;
    if (!return_notified) {
      batch.push_back(protocol::GameReturnToRoomNotification{}.Encode());
    }
    batch.push_back(RoomRoster::Snapshot(waiting, occupant.slot).Encode());
    packets.emplace_back(occupant.slot, std::move(batch));
  }
  room.record = std::move(waiting);
  room.game->Finish();
  room.finished_game = std::move(room.game);
  room.game.reset();
  room.pending_settlement.reset();
  for (auto& [slot, batch] : packets) {
    Deliver(room.sinks.at(slot), std::move(batch));
  }
}

void RoomRegistry::PollLocked(Room& room, RoomGameSession::Time now) {
  if (!room.game) {
    return;
  }
  const auto deliveries = room.game->Poll(now);
  const bool return_notified =
      std::any_of(deliveries.begin(), deliveries.end(),
                  [](const RoomGameDelivery& delivery) {
                    return delivery.packet.message_id ==
                           protocol::GameReturnToRoomNotification::kMessageId;
                  });
  DeliverGameLocked(room, deliveries);
  if (settlements_ && room.game->Phase() == RoomGamePhase::kResult &&
      !room.settlement_complete) {
    SettleLocked(room, now);
  }
  if (room.game->Phase() == RoomGamePhase::kReturning) {
    ResetGameLocked(room, return_notified);
  } else if (room.game->Phase() == RoomGamePhase::kPlaying) {
    room.record.state = RoomState::kPlaying;
  }
}

void RoomRegistry::SettleLocked(Room& room, RoomGameSession::Time now) {
  if (now < room.retry_settlement_at) {
    return;
  }
  try {
    if (!room.pending_settlement) {
      RoomMatchRequest request;
      request.match_id = room.match_id;
      request.rules = rules_;
      request.duration_milliseconds = room.duration_milliseconds;
      request.team_scores = room.game->FinalTeamScores();
      for (const auto& occupant : room.record.occupants) {
        request.members.push_back(
            {occupant.slot, occupant.account_id,
             occupant.character.character_id,
             static_cast<std::uint8_t>(occupant.member.signed_byte_at4),
             room.game->FinalScores(occupant.slot)});
      }
      request.Validate();
      room.pending_settlement = settlements_->Enqueue(std::move(request));
    }
    const auto& request = room.pending_settlement->kRequest;
    settlements_->TrySettle(room.pending_settlement, now);
    const auto completed = room.pending_settlement->Result();
    if (!completed) {
      return;
    }
    const auto& result = *completed;
    auto scores = result.Scores(request).Encode();
    auto profiles = result.Profiles(request);
    auto updated = room.record;
    for (const auto& reward : result.rewards) {
      auto* member = FindMemberLocked(room, reward.character.account_id);
      if (!member || member->slot != reward.slot) {
        throw std::logic_error("Settlement roster changed.");
      }
      for (auto& occupant : updated.occupants) {
        if (occupant.slot == reward.slot) {
          occupant.character = reward.character;
        }
      }
    }
    room.game->ConfirmSettlement(now);
    room.record = std::move(updated);
    room.settlement_complete = true;
    Broadcast(room, {std::move(scores)});
    for (auto& [slot, packet] : profiles) {
      Deliver(room.sinks.at(slot), {std::move(packet)});
    }
    if (room.settlement_delayed) {
      std::cerr << "Local match settlement recovered.\n";
    }
  } catch (const std::exception&) {
    // Keep frozen participants/scores and the same 128-bit identity.
    // Never display uncommitted rewards or restart the match on failure.
    if (!room.settlement_delayed) {
      std::cerr << "Local match settlement delayed; retrying in two seconds.\n";
    }
    room.settlement_delayed = true;
    room.retry_settlement_at = now + std::chrono::seconds{2};
  }
}

void RoomRegistry::RetrySettlements() {
  if (settlements_) {
    settlements_->ProcessPending(clock_());
  }
}

void RoomRegistry::RecoverSettlement(RoomMatchRequest request) {
  if (!settlements_) {
    throw std::logic_error("No settlement service.");
  }
  settlements_->Enqueue(std::move(request));
}

std::size_t RoomRegistry::PendingSettlementCount() const {
  return settlements_ ? settlements_->PendingCount() : 0;
}

void RoomRegistry::Poll(std::uint16_t room_id, std::uint64_t account_id) {
  std::lock_guard lock(mutex_);
  auto& room = rooms_.at(room_id);
  if (!FindMemberLocked(room, account_id)) {
    throw std::logic_error("Polling room by a non-member.");
  }
  PollLocked(room, clock_());
}

void RoomRegistry::ProcessGame(std::uint16_t room_id, std::uint64_t account_id,
                               const protocol::TcpPacket& packet) {
  std::lock_guard lock(mutex_);
  auto& room = rooms_.at(room_id);
  const auto* member = FindMemberLocked(room, account_id);
  if (!member) {
    throw std::runtime_error("Game packet outside an owned room.");
  }
  const auto slot = member->slot;
  const auto now = clock_();
  PollLocked(room, now);
  if (!room.game) {
    if (!room.finished_game) {
      throw std::runtime_error("Game packet before an owned match.");
    }
    // E090 live return crossed an in-flight movement. Validate against
    // the finished participants and discard without replay or reset.
    room.finished_game->Process(slot, packet, now);
    return;
  }
  DeliverGameLocked(room, room.game->Process(slot, packet, now));
}

void RoomRegistry::ReturnToWaiting(std::uint16_t room_id,
                                   std::uint64_t account_id) {
  std::lock_guard lock(mutex_);
  auto& room = rooms_.at(room_id);
  if (!FindMemberLocked(room, account_id) ||
      (!room.game && !room.finished_game)) {
    throw std::runtime_error("Game exit outside an owned match.");
  }
  if (room.game) {
    ResetGameLocked(room);
  }
}

RoomListPage RoomRegistry::List(std::size_t page, std::size_t page_size) const {
  if (page_size == 0) {
    throw std::invalid_argument("Room list page size is zero.");
  }
  std::lock_guard lock(mutex_);
  RoomListPage result;
  if (page > rooms_.size() / page_size) {
    return result;
  }
  auto room = rooms_.begin();
  std::advance(room, static_cast<std::ptrdiff_t>(page * page_size));
  for (; room != rooms_.end() && result.rooms.size() < page_size; ++room) {
    result.rooms.push_back(RoomRoster::Describe(room->second.record));
  }
  result.has_next_page = room != rooms_.end();
  return result;
}

std::optional<RoomRecord> RoomRegistry::Find(std::uint16_t room_id) const {
  std::lock_guard lock(mutex_);
  const auto room = rooms_.find(room_id);
  if (room == rooms_.end()) {
    return std::nullopt;
  }
  return room->second.record;
}

std::optional<std::uint16_t> RoomRegistry::FindByCharacter(
    std::uint64_t character_id) const {
  std::lock_guard lock(mutex_);
  const auto entry = by_character_.find(character_id);
  if (entry == by_character_.end() || !rooms_.contains(entry->second)) {
    return std::nullopt;
  }
  return entry->second;
}

std::size_t RoomRegistry::Count() const {
  std::lock_guard lock(mutex_);
  return rooms_.size();
}
}  // namespace bubble_fighter::server::features::rooms
