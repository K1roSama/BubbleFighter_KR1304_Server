#include "protocol/login/login_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
TcpPacket LoginResponse::Encode() const {
  if ((result == 0) != success.has_value() ||
      (national.result == 0) != national.details.has_value() ||
      (tail_marker == kExtensionMarker) != tail_value.has_value()) {
    throw std::invalid_argument(
        "Login response fields do not match their discriminators.");
  }
  PacketWriter writer;
  writer.WriteUInt32(result);
  if (success) {
    writer.WriteUtf16(success->account_identifier);
  }
  writer.WriteUInt32(identity_value);
  if (success) {
    writer.WriteUtf16(success->character_name);
    writer.WriteUInt32(success->character_type);
    writer.WriteUInt32(success->unknown_value);
    writer.WriteUInt32(success->channel_category);
    writer.WriteUInt32(success->unknown_connection_value);
    writer.WriteUInt64(success->server_time_bits);
    writer.WriteUInt32(success->game_master_value);
  }
  writer.WriteUInt32(national.result);
  if (national.details) {
    writer.WriteUInt8(national.details->byte_value);
    writer.WriteUtf16(national.details->first_string);
    writer.WriteUtf16(national.details->second_string);
    writer.WriteUInt32(national.details->boolean_value);
  }
  writer.WriteUInt8(tail_marker);
  if (tail_value) {
    writer.WriteUInt32(*tail_value);
  }
  return {kMessageId, writer.TakeBytes()};
}

LoginResponse LoginResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected login response message id.");
  }
  PacketReader reader(packet.payload);
  LoginResponse message;
  message.result = reader.ReadUInt32();
  if (message.result == 0) {
    message.success.emplace();
    message.success->account_identifier = reader.ReadUtf16();
  }
  message.identity_value = reader.ReadUInt32();
  if (message.success) {
    message.success->character_name = reader.ReadUtf16();
    message.success->character_type = reader.ReadUInt32();
    message.success->unknown_value = reader.ReadUInt32();
    message.success->channel_category = reader.ReadUInt32();
    message.success->unknown_connection_value = reader.ReadUInt32();
    message.success->server_time_bits = reader.ReadUInt64();
    message.success->game_master_value = reader.ReadUInt32();
  }
  message.national.result = reader.ReadUInt32();
  if (message.national.result == 0) {
    message.national.details.emplace();
    auto& details = *message.national.details;
    details.byte_value = reader.ReadUInt8();
    details.first_string = reader.ReadUtf16();
    details.second_string = reader.ReadUtf16();
    details.boolean_value = reader.ReadUInt32();
  }
  message.tail_marker = reader.ReadUInt8();
  if (message.tail_marker == kExtensionMarker) {
    message.tail_value = reader.ReadUInt32();
  }
  reader.RequireEnd();
  return message;
}
}  // namespace bubble_fighter::server::protocol
