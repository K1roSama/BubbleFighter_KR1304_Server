#include "protocol/inventory/item_use_arguments.h"

#include <stdexcept>

namespace bubble_fighter::server::protocol {
void ItemUseArguments::Write(PacketWriter& writer,
                             const std::vector<std::u16string>& values) {
  if (values.size() > kMaxCount) {
    throw std::invalid_argument("Too many item use arguments.");
  }
  std::size_t bytes = 12;  // two fields and vector count
  for (const auto& value : values) {
    if (value.size() > PacketWriter::kMaxStringCodeUnits ||
        4 + value.size() * 2 > kMaxPayloadBytes - bytes) {
      throw std::invalid_argument("Item use arguments exceed local limits.");
    }
    bytes += 4 + value.size() * 2;
  }
  writer.WriteUInt32(static_cast<std::uint32_t>(values.size()));
  for (const auto& value : values) {
    writer.WriteUtf16(value);
  }
}
std::vector<std::u16string> ItemUseArguments::Read(PacketReader& reader) {
  const auto count = reader.ReadUInt32();
  if (count > kMaxCount || count > reader.RemainingBytes() / 4) {
    throw std::invalid_argument("Invalid item use argument count.");
  }
  std::vector<std::u16string> values;
  values.reserve(count);
  for (std::uint32_t i = 0; i != count; ++i) {
    values.push_back(reader.ReadUtf16());
  }
  return values;
}
}  // namespace bubble_fighter::server::protocol
