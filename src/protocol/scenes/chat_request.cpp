#include "protocol/scenes/chat_request.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"
namespace bubble_fighter::server::protocol {
TcpPacket ChatRequest::Encode() const {
  PacketWriter w;
  w.WriteUInt32(channel);
  w.WriteUtf16(text);
  w.WriteUtf16(expression);
  return {kMessageId, w.TakeBytes()};
}
ChatRequest ChatRequest::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Invalid chat request.");
  }
  PacketReader r(packet.payload);
  ChatRequest m;
  m.channel = r.ReadUInt32();
  m.text = r.ReadUtf16();
  m.expression = r.ReadUtf16();
  r.RequireEnd();
  return m;
}
}  // namespace bubble_fighter::server::protocol
