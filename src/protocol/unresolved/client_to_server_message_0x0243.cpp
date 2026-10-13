#include "protocol/unresolved/client_to_server_message_0x0243.h"

#include <stdexcept>
#include <type_traits>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
namespace {
constexpr std::size_t kMaxBytes = TcpPacketCodec::kDefaultMaxPayloadSize;
void AddSize(std::size_t& size, std::size_t count, std::size_t width) {
  if (count > (kMaxBytes - size) / width) {
    throw std::length_error("C579 exceeds payload policy.");
  }
  size += count * width;
}
template <typename T>
void WriteList(PacketWriter& writer, const std::vector<T>& values) {
  writer.WriteUInt32(static_cast<std::uint32_t>(values.size()));
  for (const auto& value : values) {
    if constexpr (std::is_same_v<T, std::uint32_t>) {
      writer.WriteUInt32(value);
    } else if constexpr (std::is_same_v<T, std::uint8_t>) {
      writer.WriteUInt8(value);
    } else if constexpr (std::is_same_v<T, std::u16string>) {
      writer.WriteUtf16(value);
    } else {
      for (const auto word : value) {
        writer.WriteUInt32(word);
      }
    }
  }
}
template <typename T>
void ReadList(PacketReader& reader, std::vector<T>& values, std::size_t width) {
  const auto count = reader.ReadUInt32();
  // Check against remaining bytes before reserve, including strings'
  // minimum four-byte length field. No client count drives allocation
  // unchecked.
  if (count > reader.RemainingBytes() / width) {
    throw std::runtime_error("Truncated C579 list.");
  }
  values.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    if constexpr (std::is_same_v<T, std::uint32_t>) {
      values.push_back(reader.ReadUInt32());
    } else if constexpr (std::is_same_v<T, std::uint8_t>) {
      values.push_back(reader.ReadUInt8());
    } else if constexpr (std::is_same_v<T, std::u16string>) {
      values.push_back(reader.ReadUtf16());
    } else {
      values.push_back(
          {reader.ReadUInt32(), reader.ReadUInt32(), reader.ReadUInt32()});
    }
  }
}
}  // namespace

TcpPacket ClientToServerMessage0x0243::Encode() const {
  std::size_t size = 60;  // One scalar and fourteen independent list counts.
  for (const auto* values : {&triples_at4, &triples_at76, &triples_at124}) {
    AddSize(size, values->size(), 12);
  }
  for (const auto* values :
       {&values_at16, &values_at40, &values_at52, &values_at64, &values_at88,
        &values_at100, &values_at112, &values_at136, &values_at148}) {
    AddSize(size, values->size(), 4);
  }
  AddSize(size, bytes_at28.size(), 1);
  AddSize(size, strings_at160.size(), 4);
  for (const auto& value : strings_at160) {
    if (value.size() > PacketWriter::kMaxStringCodeUnits) {
      throw std::length_error("C579 string exceeds policy.");
    }
    AddSize(size, value.size(), 2);
  }
  PacketWriter writer;
  writer.WriteUInt32(value_at0);
  WriteList(writer, triples_at4);
  WriteList(writer, values_at16);
  WriteList(writer, bytes_at28);
  WriteList(writer, values_at40);
  WriteList(writer, values_at52);
  WriteList(writer, values_at64);
  WriteList(writer, triples_at76);
  WriteList(writer, values_at88);
  WriteList(writer, values_at100);
  WriteList(writer, values_at112);
  WriteList(writer, triples_at124);
  WriteList(writer, values_at136);
  WriteList(writer, values_at148);
  WriteList(writer, strings_at160);
  return {kMessageId, writer.TakeBytes()};
}

ClientToServerMessage0x0243 ClientToServerMessage0x0243::Decode(
    const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid C579 message id.");
  }
  if (packet.payload.size() > kMaxBytes) {
    throw std::length_error("C579 exceeds payload policy.");
  }
  PacketReader reader(packet.payload);
  ClientToServerMessage0x0243 result;
  result.value_at0 = reader.ReadUInt32();
  ReadList(reader, result.triples_at4, 12);
  ReadList(reader, result.values_at16, 4);
  ReadList(reader, result.bytes_at28, 1);
  ReadList(reader, result.values_at40, 4);
  ReadList(reader, result.values_at52, 4);
  ReadList(reader, result.values_at64, 4);
  ReadList(reader, result.triples_at76, 12);
  ReadList(reader, result.values_at88, 4);
  ReadList(reader, result.values_at100, 4);
  ReadList(reader, result.values_at112, 4);
  ReadList(reader, result.triples_at124, 12);
  ReadList(reader, result.values_at136, 4);
  ReadList(reader, result.values_at148, 4);
  ReadList(reader, result.strings_at160, 4);
  reader.RequireEnd();
  return result;
}
}  // namespace bubble_fighter::server::protocol
