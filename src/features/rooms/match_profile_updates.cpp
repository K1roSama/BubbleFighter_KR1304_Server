#include "features/rooms/match_profile_updates.h"

#include <vector>

#include "protocol/characters/user_stat_notification.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::features::rooms {
void BindMatchRewardProfiles(const MatchRewardPolicy& policy,
                             std::vector<protocol::TcpPacket>& packets) {
  for (auto& packet : packets) {
    if (packet.message_id != protocol::UserStatNotification::kMessageId) {
      continue;
    }
    auto profile = protocol::UserStatNotification::Decode(packet);
    const auto range = policy.ExperienceRange(profile.values[1]);
    profile.values[2] = range[0];
    profile.values[3] = range[1];
    packet = profile.Encode();
  }
}
}  // namespace bubble_fighter::server::features::rooms
