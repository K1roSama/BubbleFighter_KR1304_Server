#ifndef BUBBLEFIGHTER_SERVER_FEATURES_CHAT_SCENE_CHAT_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_CHAT_SCENE_CHAT_H_
#include <chrono>
#include <deque>
#include <string_view>

#include "features/characters/character_store.h"
#include "features/inventory/inventory_slot.h"
#include "protocol/scenes/chat_notification.h"
#include "protocol/scenes/chat_request.h"
namespace bubble_fighter::server::features::chat {
// Local resource limits; not recovered original server moderation rules.
class SceneChat {
 public:
  using Time = std::chrono::steady_clock::time_point;
  // Constructed after authentication and resource/ownership checks. This
  // is an internal message, never a decoded client authorization claim.
  struct Message {
    std::uint32_t channel = 0;
    std::u16string text;
    std::u16string motion;
  };
  static bool ValidText(std::u16string_view text,
                        std::size_t maximum = 256) noexcept;
  static bool NeedsInventory(std::u16string_view text) noexcept;
  static Message Prepare(
      const protocol::ChatRequest& request,
      const characters::CharacterRecord& character,
      const inventory::CharacterInventory* inventory = nullptr,
      std::uint64_t observed_at_unix_seconds = 0);
  static protocol::ChatNotification Create(std::int16_t slot,
                                           std::u16string_view trusted_name,
                                           const Message& message);
  static protocol::TcpPacket Notice(std::u16string_view text);
  bool Admit(Time now);

 private:
  std::deque<Time> sent_;
};
}  // namespace bubble_fighter::server::features::chat

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_CHAT_SCENE_CHAT_H_
