#include "protocol/scenes/scene_descriptor_notification.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"
namespace bubble_fighter::server::protocol {
TcpPacket SceneDescriptorNotification::Encode() const {
  PacketWriter w;
  descriptor.Write(w);
  return {kMessageId, w.TakeBytes()};
}
SceneDescriptorNotification SceneDescriptorNotification::Decode(
    const TcpPacket& p) {
  if (p.message_id != kMessageId) {
    throw std::invalid_argument("Invalid scene descriptor message.");
  }
  PacketReader r(p.payload);
  SceneDescriptorNotification m{SessionDescriptor::Read(r)};
  r.RequireEnd();
  return m;
}
}  // namespace bubble_fighter::server::protocol
