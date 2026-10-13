#ifndef BUBBLEFIGHTER_SERVER_FEATURES_LOBBY_PLAZA_REGISTRY_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_LOBBY_PLAZA_REGISTRY_H_
#include <map>
#include <mutex>

#include "features/chat/scene_chat.h"
#include "features/rooms/room_packet_queue.h"
#include "features/scenes/scene_roster.h"
#include "protocol/scenes/plaza_movement.h"

namespace bubble_fighter::server::features::lobby {
class PlazaRegistry;
class PlazaLease {
 public:
  PlazaLease() = default;
  PlazaLease(PlazaLease&& other) noexcept;
  PlazaLease& operator=(PlazaLease&& other) noexcept;
  PlazaLease(const PlazaLease&) = delete;
  PlazaLease& operator=(const PlazaLease&) = delete;
  ~PlazaLease();
  explicit operator bool() const noexcept;
  void Release() noexcept;
  void Move(const protocol::PlazaMovementRequest& movement);
  void Refresh(scenes::SceneSlot player, bool send_snapshot);
  void Chat(const features::chat::SceneChat::Message& request);

 private:
  friend class PlazaRegistry;
  PlazaLease(std::shared_ptr<PlazaRegistry> registry, std::uint16_t scene,
             std::uint64_t account) noexcept;
  std::shared_ptr<PlazaRegistry> registry_;
  std::uint16_t scene_ = 0;
  std::uint64_t account_ = 0;
};

// One registry for the server's current local channel. First non-full
// plaza, lowest vacant slot; capacity 16 is the user's local policy.
// Ordered delivery under the lock uses enqueue-only, nonblocking sinks.
class PlazaRegistry : public std::enable_shared_from_this<PlazaRegistry> {
 public:
  static constexpr std::size_t kCapacity = 16;
  PlazaLease Enter(protocol::GotoRoomNotification envelope,
                   scenes::SceneSlot player, std::uint64_t account,
                   rooms::RoomPacketSink sink);

 private:
  friend class PlazaLease;
  struct Member {
    std::uint32_t slot = 0;
    scenes::SceneSlot player;
    rooms::RoomPacketSink sink;
    std::optional<protocol::PlazaMovementRequest> movement;
  };
  struct Scene {
    protocol::GotoRoomNotification envelope;
    std::map<std::uint64_t, Member> members;
  };
  static protocol::GotoRoomNotification Snapshot(const Scene& scene,
                                                 std::uint64_t account);
  static protocol::TcpPacket Update(
      const protocol::GotoRoomNotification& snapshot);
  static void Deliver(const Member& member,
                      std::vector<protocol::TcpPacket> packets) noexcept;
  void Leave(std::uint16_t scene, std::uint64_t account) noexcept;
  void Move(std::uint16_t scene, std::uint64_t account,
            const protocol::PlazaMovementRequest& movement);
  void Refresh(std::uint16_t scene, std::uint64_t account,
               scenes::SceneSlot player, bool send_snapshot);
  void Chat(std::uint16_t scene, std::uint64_t account,
            const features::chat::SceneChat::Message& request);
  std::mutex mutex_;
  std::map<std::uint16_t, Scene> scenes_;
};
}  // namespace bubble_fighter::server::features::lobby

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_LOBBY_PLAZA_REGISTRY_H_
