#include "protocol/scenes/chat_notification.h"

#include <bit>
#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"
namespace bubble_fighter::server::protocol {
TcpPacket ChatNotification::Encode() const {
  PacketWriter w;
  w.WriteUInt16(std::bit_cast<std::uint16_t>(slot));
  w.WriteUtf16(sender);
  w.WriteUInt32(channel);
  w.WriteUInt32(value_at32);
  w.WriteUtf16(expression);
  w.WriteUtf16(text);
  w.WriteUInt32(text_color);
  w.WriteUInt32(name_color);
  w.WriteUInt32(flag_at92 ? 1U : 0U);
  w.WriteUtf16(string_at96);
  w.WriteUInt32(flag_at120 ? 1U : 0U);
  return {kMessageId, w.TakeBytes()};
}
ChatNotification ChatNotification::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid chat notification.");
  }
  PacketReader r(packet.payload);
  ChatNotification m;
  m.slot = std::bit_cast<std::int16_t>(r.ReadUInt16());
  m.sender = r.ReadUtf16();
  m.channel = r.ReadUInt32();
  m.value_at32 = r.ReadUInt32();
  m.expression = r.ReadUtf16();
  m.text = r.ReadUtf16();
  m.text_color = r.ReadUInt32();
  m.name_color = r.ReadUInt32();
  m.flag_at92 = r.ReadUInt32() != 0;
  m.string_at96 = r.ReadUtf16();
  m.flag_at120 = r.ReadUInt32() != 0;
  r.RequireEnd();
  return m;
}
}  // namespace bubble_fighter::server::protocol
