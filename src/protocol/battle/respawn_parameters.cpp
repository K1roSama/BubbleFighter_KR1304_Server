#include "protocol/battle/respawn_parameters.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
namespace {
void WriteOptional(PacketWriter& writer,
                   const std::optional<std::uint32_t>& value) {
  writer.WriteUInt8(value ? 0x3A : 0xBA);
  if (value) {
    writer.WriteUInt32(*value);
  }
}
std::optional<std::uint32_t> ReadOptional(PacketReader& reader) {
  const auto marker = reader.ReadUInt8();
  if (marker == 0x3A) {
    return reader.ReadUInt32();
  }
  if (marker == 0xBA) {
    return std::nullopt;
  }
  // Native readers treat every non-0x3A marker as absent. This
  // server accepts only markers produced by the confirmed writer.
  throw std::runtime_error("Noncanonical respawn optional marker.");
}
}  // namespace

void RespawnParameters::WriteTo(PacketWriter& writer) const {
  writer.WriteUInt32(actor_object_id);
  WriteOptional(writer, value_at4);
  WriteOptional(writer, value_at12);
}
RespawnParameters RespawnParameters::ReadFrom(PacketReader& reader) {
  RespawnParameters result;
  result.actor_object_id = reader.ReadUInt32();
  result.value_at4 = ReadOptional(reader);
  result.value_at12 = ReadOptional(reader);
  return result;
}
}  // namespace bubble_fighter::server::protocol
