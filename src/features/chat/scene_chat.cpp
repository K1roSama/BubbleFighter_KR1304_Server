#include "features/chat/scene_chat.h"

#include <algorithm>
#include <stdexcept>

#include "features/chat/generated/chat_expression_data.h"
#include "features/inventory/item_catalog.h"
namespace bubble_fighter::server::features::chat {
bool SceneChat::ValidText(std::u16string_view text,
                          std::size_t maximum) noexcept {
  if (text.empty() || text.size() > maximum) {
    return false;
  }
  bool visible = false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const auto c = text[i];
    if (c < 0x20 || c == 0x7F || c == 0x2028 || c == 0x2029 ||
        (c >= 0x202A && c <= 0x202E) || (c >= 0x2066 && c <= 0x2069)) {
      return false;
    }
    if (c >= 0xD800 && c <= 0xDBFF) {
      if (++i == text.size() || text[i] < 0xDC00 || text[i] > 0xDFFF) {
        return false;
      }
    } else if (c >= 0xDC00 && c <= 0xDFFF) {
      return false;
    }
    if (c != u' ' && c != 0x3000) {
      visible = true;
    }
  }
  return visible;
}
bool SceneChat::NeedsInventory(std::u16string_view text) noexcept {
  return std::any_of(expression_data::kEntries.begin(),
                     expression_data::kEntries.end(), [&](const auto& entry) {
                       return entry.grant_count != 0 &&
                              text.find(entry.keyword) != text.npos;
                     });
}
SceneChat::Message SceneChat::Prepare(
    const protocol::ChatRequest& request,
    const characters::CharacterRecord& character,
    const inventory::CharacterInventory* inventory,
    std::uint64_t observed_at_unix_seconds) {
  if (request.channel > 1 || !ValidText(request.text)) {
    throw std::invalid_argument("Unsupported scene chat.");
  }
  // 99F1C0/9246B0 can intentionally suppress an otherwise matching
  // keyword action. Empty means text only; do not reconstruct a motion.
  if (request.expression.empty()) {
    return {request.channel, request.text, {}};
  }
  if (inventory) {
    if (inventory->account_id != character.account_id ||
        inventory->character_id != character.character_id) {
      throw std::invalid_argument("Chat expression inventory owner mismatch.");
    }
    (void)inventory->ToRecords(character.account_id, observed_at_unix_seconds);
  }
  const auto owned = [&](std::uint32_t item) {
    if (!inventory) {
      return false;
    }
    const auto* definition = inventory::ItemCatalog::Find(item);
    return definition &&
           std::any_of(
               inventory->slots.begin(), inventory->slots.end(),
               [&](const auto& slot) {
                 if (slot.item_id != item || slot.usage != definition->usage) {
                   return false;
                 }
                 return slot.usage == inventory::InventoryUsage::kDuration
                            ? (!slot.expires_at_unix_seconds ||
                               *slot.expires_at_unix_seconds >
                                   observed_at_unix_seconds)
                            : slot.quantity != 0;
               });
  };
  for (const auto& entry : expression_data::kEntries) {
    if (entry.motion != request.expression ||
        request.text.find(entry.keyword) == request.text.npos) {
      continue;
    }
    bool permitted = entry.grant_count == 0;
    for (std::size_t n = 0; n < entry.grant_count; ++n) {
      const auto& grant = expression_data::kGrants[entry.grant_offset + n];
      if (grant.character == character.character_type && owned(grant.item)) {
        permitted = true;
        break;
      }
    }
    if (!permitted) {
      continue;
    }
    // Preserve the native unordered-table choice, but only when its
    // keyword and character/item grant match trusted server state.
    return {request.channel, request.text, std::u16string(entry.motion)};
  }
  return {request.channel, request.text, {}};
}
protocol::ChatNotification SceneChat::Create(std::int16_t slot,
                                             std::u16string_view trusted_name,
                                             const Message& message) {
  if (message.channel > 1 || !ValidText(message.text)) {
    throw std::invalid_argument("Unsupported scene chat.");
  }
  protocol::ChatNotification m;
  m.slot = slot;
  m.sender = trusted_name;
  m.channel = message.channel;
  m.text = message.text;
  if (slot >= 0) {
    // BFCLIENT02 9EAC70 checks marker "e" and plays the optional motion
    // from S509 +96. C502's second string is not S509's marker field.
    m.expression = u"e";
    m.string_at96 = message.motion;
  }
  return m;
}
protocol::TcpPacket SceneChat::Notice(std::u16string_view text) {
  protocol::ChatNotification m;
  m.sender = u"系统";
  m.text = text;
  return m.Encode();
}
bool SceneChat::Admit(Time now) {
  using namespace std::chrono_literals;
  if (!sent_.empty() && now < sent_.back()) {
    return false;
  }
  while (!sent_.empty() && now - sent_.front() >= 3s) {
    sent_.pop_front();
  }
  if (sent_.size() >= 5) {
    return false;
  }
  sent_.push_back(now);
  return true;
}
}  // namespace bubble_fighter::server::features::chat
