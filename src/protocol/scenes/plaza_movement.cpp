#include "protocol/scenes/plaza_movement.h"

#include <bit>
#include <cmath>
#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"
namespace bubble_fighter::server::protocol {
namespace {
void Write(PacketWriter& w, const PlazaMovementRequest& m) {
  m.Validate();
  w.WriteUInt32(m.sequence);
  for (auto v : m.position_bits) {
    w.WriteUInt32(v);
  }
  for (auto v : m.axes) {
    w.WriteUInt8(v);
  }
  w.WriteUInt32(m.yaw_bits);
}
PlazaMovementRequest Read(PacketReader& r) {
  PlazaMovementRequest m;
  m.sequence = r.ReadUInt32();
  for (auto& v : m.position_bits) {
    v = r.ReadUInt32();
  }
  for (auto& v : m.axes) {
    v = r.ReadUInt8();
  }
  m.yaw_bits = r.ReadUInt32();
  m.Validate();
  return m;
}
}  // namespace
void PlazaMovementRequest::Validate() const {
  // Local input bounds, not recovered map collision or original policy.
  const auto valid = [](std::uint32_t v) {
    const auto f = std::bit_cast<float>(v);
    return std::isfinite(f) && std::abs(f) <= 10000000.0F;
  };
  for (auto v : position_bits) {
    if (!valid(v)) {
      throw std::invalid_argument("Invalid plaza position.");
    }
  }
  if (!valid(yaw_bits)) {
    throw std::invalid_argument("Invalid plaza yaw.");
  }
  for (auto v : axes) {
    if (v != 0 && v != 1 && v != 255) {
      throw std::invalid_argument("Invalid plaza axis.");
    }
  }
}
TcpPacket PlazaMovementRequest::Encode() const {
  PacketWriter w;
  Write(w, *this);
  return {kMessageId, w.TakeBytes()};
}
PlazaMovementRequest PlazaMovementRequest::Decode(const TcpPacket& p) {
  if (p.message_id != kMessageId) {
    throw std::invalid_argument("Invalid plaza movement request.");
  }
  PacketReader r(p.payload);
  auto m = Read(r);
  r.RequireEnd();
  return m;
}
TcpPacket PlazaMovementNotification::Encode() const {
  PacketWriter w;
  w.WriteUInt32(player_id);
  Write(w, movement);
  return {kMessageId, w.TakeBytes()};
}
PlazaMovementNotification PlazaMovementNotification::Decode(
    const TcpPacket& p) {
  if (p.message_id != kMessageId) {
    throw std::invalid_argument("Invalid plaza movement notification.");
  }
  PacketReader r(p.payload);
  PlazaMovementNotification m;
  m.player_id = r.ReadUInt32();
  m.movement = Read(r);
  r.RequireEnd();
  return m;
}
}  // namespace bubble_fighter::server::protocol
