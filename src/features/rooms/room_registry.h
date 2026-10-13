#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_REGISTRY_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_REGISTRY_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "features/characters/character_store.h"
#include "features/chat/scene_chat.h"
#include "features/rooms/room_game_session.h"
#include "features/rooms/room_settlement_queue.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/scenes/goto_room_notification.h"
#include "protocol/scenes/session_descriptor.h"

namespace bubble_fighter::server::features::rooms {
// Delivery to one connection. The registry calls it while holding its
// lock, so every member observes a room's events in the same order and a
// member's own result is never overtaken by a later broadcast. It must
// only queue the packets: no blocking on a socket and no call back into
// the registry. Exceptions are swallowed (the connection is closing).
using RoomPacketSink = std::function<void(std::vector<protocol::TcpPacket>)>;

// Descriptor ValueAt4, which is room context +12 (RoomCreation R20):
// 0x00970590 only sends a start request while it is 2, and 0x0094D7A0
// names 4 "Play" (5 is "Gameover", not produced by this server).
enum class RoomState : std::uint32_t {
  kWaiting = 2,
  kLoading = 3,
  kPlaying = 4
};

struct RoomOccupant {
  std::uint32_t slot = 0;
  std::uint64_t account_id = 0;
  characters::CharacterRecord character;
  // Trusted plaza member state. BooleansAt96 carries the C505 room state
  // (+96 ready, +97 state 2, +98 state 3; RoomCreation R19).
  protocol::SessionMember member;
  protocol::PartySupplementaryRecord equipment;
  bool operator==(const RoomOccupant&) const = default;
};

// Server-side room state. Settings are the host's accepted creation
// settings; RoomRoster derives every sent descriptor from them. The
// password is compared on join and must never be logged.
struct RoomRecord {
  std::uint16_t room_id = 0;
  std::uint32_t slot_count = 0;
  std::uint32_t host_slot = 0;
  RoomState state = RoomState::kWaiting;
  protocol::SessionDescriptor settings;
  std::u16string password;
  // Sent unchanged in S117/S503/S511; no team assignment is implemented.
  protocol::SessionTeam team;
  // Ascending by slot; never empty for a published room.
  std::vector<RoomOccupant> occupants;

  const RoomOccupant* FindSlot(std::uint32_t slot) const noexcept;
  bool operator==(const RoomRecord&) const = default;
};

class RoomRegistry;

struct RoomInventoryChange {
  characters::CharacterRecord character;
  protocol::PartySupplementaryRecord equipment;
};

enum class RoomStartStatus {
  kStarted,
  kNotHost,
  kNotWaiting,
  kNotEnoughPlayers,
  kMembersNotReady,
  kUnsupportedMap
};

// One member's place in a room. Releasing it (or destroying it, e.g. on
// disconnect) leaves the room: the others receive S511 and, when the
// host left, S502; the last member closes the room.
class RoomLease {
 public:
  RoomLease() = default;
  RoomLease(RoomLease&& other) noexcept;
  RoomLease& operator=(RoomLease&& other) noexcept;
  RoomLease(const RoomLease&) = delete;
  RoomLease& operator=(const RoomLease&) = delete;
  ~RoomLease();

  std::uint16_t RoomId() const noexcept;
  explicit operator bool() const noexcept;
  void Release() noexcept;

  // Stores the member's C505 state and broadcasts S503. Returns false
  // (nothing sent) while the room is playing.
  bool SetMemberState(const std::array<std::uint32_t, 3>& state);
  // Host only, while waiting, with every other member ready and at least
  // minimumPlayers members: marks the room playing and broadcasts packet.
  RoomStartStatus Start(const protocol::TcpPacket& packet,
                        std::size_t minimum_players,
                        std::uint32_t duration_milliseconds,
                        std::u16string_view trusted_map_key = {});
  void ProcessGame(const protocol::TcpPacket& packet);
  void Poll();
  void ReturnToWaiting();
  std::optional<characters::CharacterRecord> Character() const;
  void Chat(const features::chat::SceneChat::Message& request);
  void RefreshCharacter(const characters::CharacterRecord& character);
  bool ChangeInventory(const std::function<RoomInventoryChange()>& change);

 private:
  friend class RoomReservation;
  friend class RoomRegistry;
  RoomLease(std::shared_ptr<RoomRegistry> registry, std::uint16_t room_id,
            std::uint64_t account_id) noexcept;
  std::shared_ptr<RoomRegistry> registry_;
  std::uint16_t room_id_ = 0;
  std::uint64_t account_id_ = 0;
};

// Exclusive claim on an identity and an owner before anything is encoded.
// Nothing is visible to Find/Count/List until Commit; destruction rolls back.
class RoomReservation {
 public:
  RoomReservation(RoomReservation&& other) noexcept;
  RoomReservation& operator=(RoomReservation&& other) noexcept;
  RoomReservation(const RoomReservation&) = delete;
  RoomReservation& operator=(const RoomReservation&) = delete;
  ~RoomReservation();

  std::uint16_t RoomId() const noexcept;
  // The record must use the reserved identity with the reserved owner
  // as its only occupant, in the host slot. Publishing and delivering
  // hostPackets to hostSink happen under one lock. On failure the
  // reservation stays intact.
  RoomLease Commit(RoomRecord record, RoomPacketSink host_sink,
                   std::vector<protocol::TcpPacket> host_packets) &&;

 private:
  friend class RoomRegistry;
  RoomReservation(std::shared_ptr<RoomRegistry> registry, std::uint16_t room_id,
                  std::uint64_t account_id,
                  std::uint64_t character_id) noexcept;
  void Abandon() noexcept;
  std::shared_ptr<RoomRegistry> registry_;
  std::uint16_t room_id_ = 0;
  std::uint64_t account_id_ = 0;
  std::uint64_t character_id_ = 0;
};

enum class RoomReserveStatus { kReserved, kAlreadyInRoom, kRegistryFull };

struct RoomReserveResult {
  RoomReserveStatus status = RoomReserveStatus::kRegistryFull;
  std::optional<RoomReservation> reservation;
};

enum class RoomJoinStatus {
  kJoined,
  kNoSuchRoom,
  kPlaying,
  kWrongPassword,
  kFull
};

struct RoomJoinResult {
  RoomJoinStatus status = RoomJoinStatus::kNoSuchRoom;
  // Present only when joined.
  RoomLease room;
};

struct RoomListPage {
  std::vector<protocol::SessionDescriptor> rooms;
  bool has_next_page = false;
};

// Process-wide room table shared by all connections; thread safe.
class RoomRegistry : public std::enable_shared_from_this<RoomRegistry> {
 public:
  // 0x00855310 sign-extends descriptor ShortAt0 into the C403 join
  // request, so identities stay in the positive int16 range.
  static constexpr std::uint16_t kMaxRoomId = 0x7FFF;

  // maxRooms is a local resource policy in [1, MaxRoomId].
  using GameClock = std::function<RoomGameSession::Time()>;
  using SeedFactory = std::function<std::uint32_t()>;
  static std::shared_ptr<RoomRegistry> Create(std::size_t max_rooms,
                                              GameClock clock = {},
                                              SeedFactory seed = {},
                                              RoomMatchSettler settler = {},
                                              MatchRewardPolicy rules = {});

  // An account or character is in at most one room or pending claim.
  RoomReserveResult Reserve(std::uint64_t account_id,
                            std::uint64_t character_id);
  // Takes the lowest free slot. The joiner receives joinerPrefix, then
  // its room S117 (local slot = its slot); the others receive S503.
  // Throws when the joiner is already in a room or claim.
  RoomJoinResult Join(std::uint16_t room_id, const std::u16string& password,
                      RoomOccupant joiner, RoomPacketSink sink,
                      std::vector<protocol::TcpPacket> joiner_prefix);
  // Rooms in identity order; pageSize > 0.
  RoomListPage List(std::size_t page, std::size_t page_size) const;
  std::optional<RoomRecord> Find(std::uint16_t room_id) const;
  std::optional<std::uint16_t> FindByCharacter(
      std::uint64_t character_id) const;
  std::size_t Count() const;
  // Application-owned worker also calls this when there are no clients.
  void RetrySettlements();
  void RecoverSettlement(RoomMatchRequest request);
  std::size_t PendingSettlementCount() const;

 private:
  friend class RoomLease;
  friend class RoomReservation;
  struct Owner {
    std::uint64_t account_id = 0;
    std::uint64_t character_id = 0;
  };
  struct Room {
    RoomRecord record;
    std::map<std::uint32_t, RoomPacketSink> sinks;
    std::optional<RoomGameSession> game;
    std::optional<RoomGameSession> finished_game;
    std::array<std::uint8_t, 16> match_id{};
    std::uint32_t duration_milliseconds = 0;
    std::shared_ptr<RoomSettlementQueue::Job> pending_settlement;
    bool settlement_complete = false, settlement_delayed = false;
    std::set<std::uint64_t> inventory_changes;
    RoomGameSession::Time retry_settlement_at{};
  };
  explicit RoomRegistry(std::size_t max_rooms, GameClock clock,
                        SeedFactory seed, RoomMatchSettler settler,
                        MatchRewardPolicy rules);
  void Publish(std::uint16_t room_id, std::uint64_t account_id,
               std::uint64_t character_id, RoomRecord record,
               RoomPacketSink host_sink,
               std::vector<protocol::TcpPacket> host_packets);
  void Abandon(std::uint16_t room_id) noexcept;
  void Leave(std::uint16_t room_id, std::uint64_t account_id) noexcept;
  bool SetMemberState(std::uint16_t room_id, std::uint64_t account_id,
                      const std::array<std::uint32_t, 3>& state);
  RoomStartStatus Start(std::uint16_t room_id, std::uint64_t account_id,
                        const protocol::TcpPacket& packet,
                        std::size_t minimum_players,
                        std::uint32_t duration_milliseconds,
                        std::u16string_view trusted_map_key);
  void ProcessGame(std::uint16_t room_id, std::uint64_t account_id,
                   const protocol::TcpPacket& packet);
  void Poll(std::uint16_t room_id, std::uint64_t account_id);
  void ReturnToWaiting(std::uint16_t room_id, std::uint64_t account_id);
  void Chat(std::uint16_t room_id, std::uint64_t account_id,
            const features::chat::SceneChat::Message& request);
  void RefreshCharacter(std::uint16_t room_id, std::uint64_t account_id,
                        const characters::CharacterRecord& character);
  bool ChangeInventory(std::uint16_t room_id, std::uint64_t account_id,
                       const std::function<RoomInventoryChange()>& change);
  void PollLocked(Room& room, RoomGameSession::Time now);
  void SettleLocked(Room& room, RoomGameSession::Time now);
  static void ResetGameLocked(Room& room, bool return_notified = false);
  static void DeliverGameLocked(
      const Room& room, const std::vector<RoomGameDelivery>& deliveries);
  bool IsUsedLocked(std::uint16_t room_id) const;
  RoomOccupant* FindMemberLocked(Room& room, std::uint64_t account_id);
  static void Deliver(const RoomPacketSink& sink,
                      std::vector<protocol::TcpPacket> packets) noexcept;
  static void Broadcast(
      const Room& room, const std::vector<protocol::TcpPacket>& packets,
      std::optional<std::uint32_t> except_slot = std::nullopt) noexcept;

  mutable std::mutex mutex_;
  std::size_t max_rooms_ = 0;
  std::uint16_t next_room_id_ = 1;
  GameClock clock_;
  SeedFactory seed_;
  std::shared_ptr<RoomSettlementQueue> settlements_;
  MatchRewardPolicy rules_;
  std::map<std::uint16_t, Owner> pending_;
  std::map<std::uint16_t, Room> rooms_;
  std::unordered_map<std::uint64_t, std::uint16_t> by_account_;
  std::unordered_map<std::uint64_t, std::uint16_t> by_character_;
};
}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_REGISTRY_H_
