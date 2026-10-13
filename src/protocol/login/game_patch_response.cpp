#include "protocol/login/game_patch_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket GamePatchResponse::Encode() const {
  PacketWriter writer;
  writer.WriteUInt32(result);
  writer.WriteUtf16(message);
  return {kMessageId, writer.TakeBytes()};
}

GamePatchResponse GamePatchResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected game patch response id.");
  }
  PacketReader reader(packet.payload);
  GamePatchResponse message;
  message.result = reader.ReadUInt32();
  message.message = reader.ReadUtf16();
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
