#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_PACKET_QUEUE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_PACKET_QUEUE_H_
#include <memory>
#include <mutex>

#include "features/rooms/room_registry.h"

namespace bubble_fighter::server::features::rooms {
// Registry callbacks only enqueue. The connection thread owns encoding,
// encryption and socket writes. Overflow fails the connection, not just a
// packet.
class RoomPacketQueue : public std::enable_shared_from_this<RoomPacketQueue> {
 public:
  RoomPacketSink Sink();
  std::vector<protocol::TcpPacket> Drain();
  void Clear();

 private:
  void Push(std::vector<protocol::TcpPacket> packets) noexcept;
  std::mutex mutex_;
  std::vector<protocol::TcpPacket> packets_;
  std::size_t bytes_ = 0;
  bool failed_ = false;
};
}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_ROOM_PACKET_QUEUE_H_
