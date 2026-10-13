#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_SCENE_DESCRIPTOR_NOTIFICATION_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_SCENE_DESCRIPTOR_NOTIFICATION_H_
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/scenes/session_descriptor.h"
namespace bubble_fighter::server::protocol {
// 9D9930 case 501 -> 113BD20, the shared S117 descriptor reader.
struct SceneDescriptorNotification {
  static constexpr std::uint16_t kMessageId = 501;
  SessionDescriptor descriptor;
  TcpPacket Encode() const;
  static SceneDescriptorNotification Decode(const TcpPacket& packet);
  bool operator==(const SceneDescriptorNotification&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_SCENES_SCENE_DESCRIPTOR_NOTIFICATION_H_
