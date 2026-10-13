#include "protocol/battle/compressed_shot_record.h"

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
void CompressedShotRecord::Encode(PacketWriter& writer) const {
  writer.WriteUInt32(actor_object_id);
  writer.WriteUInt8(weapon_index);
  for (const auto value : position) {
    writer.WriteUInt16(value);
  }
  for (const auto value : vector_at12) {
    writer.WriteUInt16(value);
  }
  writer.WriteUInt32(value_at20);
  writer.WriteUInt8(byte_at24);
  writer.WriteUInt32(shot_sequence);
  writer.WriteUInt32(packed_time_at32);
  writer.WriteUInt16(short_at36);
}

CompressedShotRecord CompressedShotRecord::Decode(PacketReader& reader) {
  CompressedShotRecord value;
  value.actor_object_id = reader.ReadUInt32();
  value.weapon_index = reader.ReadUInt8();
  for (auto& coordinate : value.position) {
    coordinate = reader.ReadUInt16();
  }
  for (auto& coordinate : value.vector_at12) {
    coordinate = reader.ReadUInt16();
  }
  value.value_at20 = reader.ReadUInt32();
  value.byte_at24 = reader.ReadUInt8();
  value.shot_sequence = reader.ReadUInt32();
  value.packed_time_at32 = reader.ReadUInt32();
  value.short_at36 = reader.ReadUInt16();
  return value;
}

bool CompressedShotRecord::HasFiniteCoordinates() const noexcept {
  // Native FA5360 decodes these as IEEE binary16. All-ones exponent
  // produces infinity/NaN; the codec preserves it, gameplay rejects it.
  for (const auto value : position) {
    if ((value & 0x7C00U) == 0x7C00U) {
      return false;
    }
  }
  for (const auto value : vector_at12) {
    if ((value & 0x7C00U) == 0x7C00U) {
      return false;
    }
  }
  return true;
}
}  // namespace bubble_fighter::server::protocol
