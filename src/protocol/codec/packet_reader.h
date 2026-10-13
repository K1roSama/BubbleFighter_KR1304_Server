#ifndef BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_PACKET_READER_H_
#define BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_PACKET_READER_H_

#include <cstdint>
#include <span>
#include <string>

namespace bubble_fighter::server::protocol {
class PacketReader {
 public:
  explicit PacketReader(std::span<const std::uint8_t> bytes);
  std::uint8_t ReadUInt8();
  std::uint16_t ReadUInt16();
  std::uint32_t ReadUInt32();
  std::uint64_t ReadUInt64();
  std::u16string ReadUtf16();
  void RequireEnd() const;
  std::size_t RemainingBytes() const noexcept;

 private:
  std::span<const std::uint8_t> ReadBytes(std::size_t size);
  std::span<const std::uint8_t> remaining_;
};
}  // namespace bubble_fighter::server::protocol

#endif  // BUBBLEFIGHTER_SERVER_PROTOCOL_CODEC_PACKET_READER_H_
