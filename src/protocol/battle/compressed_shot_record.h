#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_COMPRESSED_SHOT_RECORD_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_COMPRESSED_SHOT_RECORD_H_
#include <array>
#include <cstdint>

namespace bubble_fighter::server::protocol {
class PacketReader;
class PacketWriter;

// E086: 32 wire bytes. At offsets refer to the client's padded record.
// Keep binary16 bits verbatim, including signed zero and NaN payloads.
struct CompressedShotRecord {
  std::uint32_t actor_object_id = 0;
  std::uint8_t weapon_index = 0;
  std::array<std::uint16_t, 3> position{};
  std::array<std::uint16_t, 3> vector_at12{};
  std::uint32_t value_at20 = 0;
  std::uint8_t byte_at24 = 0;
  std::uint32_t shot_sequence = 0;
  std::uint32_t packed_time_at32 = 0;
  std::uint16_t short_at36 = 0;
  void Encode(PacketWriter& writer) const;
  static CompressedShotRecord Decode(PacketReader& reader);
  bool HasFiniteCoordinates() const noexcept;
  bool operator==(const CompressedShotRecord&) const = default;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_BATTLE_COMPRESSED_SHOT_RECORD_H_
