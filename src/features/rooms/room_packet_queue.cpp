#include "features/rooms/room_packet_queue.h"

#include <stdexcept>
#include <utility>

namespace bubble_fighter::server::features::rooms {
RoomPacketSink RoomPacketQueue::Sink() {
  std::weak_ptr<RoomPacketQueue> self = shared_from_this();
  return [self](std::vector<protocol::TcpPacket> packets) {
    if (auto queue = self.lock()) {
      queue->Push(std::move(packets));
    }
  };
}
void RoomPacketQueue::Push(std::vector<protocol::TcpPacket> packets) noexcept {
  std::lock_guard lock(mutex_);
  if (failed_) {
    return;
  }
  try {
    for (auto& packet : packets) {
      if (packets_.size() >= 1024 ||
          packet.payload.size() > 1024 * 1024 - bytes_) {
        throw std::runtime_error("Room delivery backlog exceeded.");
      }
      bytes_ += packet.payload.size();
      packets_.push_back(std::move(packet));
    }
  } catch (...) {
    failed_ = true;
    packets_.clear();
    bytes_ = 0;
  }
}
std::vector<protocol::TcpPacket> RoomPacketQueue::Drain() {
  std::lock_guard lock(mutex_);
  if (failed_) {
    throw std::runtime_error("Room delivery queue failed.");
  }
  std::vector<protocol::TcpPacket> result;
  result.swap(packets_);
  bytes_ = 0;
  return result;
}
void RoomPacketQueue::Clear() {
  std::lock_guard lock(mutex_);
  packets_.clear();
  bytes_ = 0;
}
}  // namespace bubble_fighter::server::features::rooms
