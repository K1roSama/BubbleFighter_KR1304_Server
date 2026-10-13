#ifndef BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_MATCH_PROFILE_UPDATES_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_MATCH_PROFILE_UPDATES_H_

#include "features/rooms/match_reward_policy.h"
#include "protocol/codec/tcp_packet_codec.h"

namespace bubble_fighter::server::features::rooms {

// S105 range bindings recovered in RoomSnapshot. All application delivery
// paths use this policy, including menu, purchase and match settlement.
void BindMatchRewardProfiles(const MatchRewardPolicy& policy,
                             std::vector<protocol::TcpPacket>& packets);

}  // namespace bubble_fighter::server::features::rooms

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_ROOMS_MATCH_PROFILE_UPDATES_H_
