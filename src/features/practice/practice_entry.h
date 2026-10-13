#ifndef BUBBLEFIGHTER_SERVER_FEATURES_PRACTICE_PRACTICE_ENTRY_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_PRACTICE_PRACTICE_ENTRY_H_

#include "protocol/unresolved/client_to_server_message_0x006b.h"
#include "sessions/menu_initialization.h"

namespace bubble_fighter::server::features::practice {
class PracticeEntry {
 public:
  // E070 local policy: the current selector-zero map enters native mode
  // 40 (GameTrainingRoomStrategy). Original server selection is unknown.
  // Resource fields are pinned to BFCLIENT02 / version.dat 1304.
  static void Validate(const protocol::ClientToServerMessage0x006B& request);
  static std::vector<protocol::TcpPacket> Prepare(
      const protocol::ClientToServerMessage0x006B& request,
      const characters::CharacterRecord& character,
      std::uint64_t authenticated_account_id,
      sessions::MenuInitializationData data,
      std::uint64_t observed_at_unix_seconds);
};
}  // namespace bubble_fighter::server::features::practice

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_PRACTICE_PRACTICE_ENTRY_H_
