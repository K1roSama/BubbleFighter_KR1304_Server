#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_RESPAWN_PARAMETERS_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_RESPAWN_PARAMETERS_H_
#include <cstdint>
#include <optional>

namespace bubble_fighter::server::protocol {
class PacketReader;
class PacketWriter;

// E080: shared 0x010C0430 writer / 0x010C04D0 reader. Optional
// field meanings are unresolved; offsets refer to the x86 object.
struct RespawnParameters {
  std::uint32_t actor_object_id = 0;
  std::optional<std::uint32_t> value_at4;
  std::optional<std::uint32_t> value_at12;

  void WriteTo(PacketWriter& writer) const;
  static RespawnParameters ReadFrom(PacketReader& reader);
  bool operator==(const RespawnParameters&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_RESPAWN_PARAMETERS_H_
