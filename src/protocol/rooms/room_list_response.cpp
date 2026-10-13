#include "protocol/rooms/room_list_response.h"

#include <stdexcept>

#include "protocol/codec/packet_reader.h"
#include "protocol/codec/packet_writer.h"

namespace bubble_fighter::server::protocol {
namespace {
// Smallest SessionDescriptor encoding: fixed fields plus empty strings
// and collections; guards the count before reserving.
constexpr std::size_t kMinimumDescriptorBytes = 28;
}  // namespace

TcpPacket RoomListResponse::Encode() const {
  if (rooms.size() > kMaxRooms) {
    throw std::length_error("Room list exceeds local limit.");
  }
  PacketWriter writer;
  writer.WriteUInt32(page);
  writer.WriteUInt32(has_next_page);
  writer.WriteUInt32(value_at8);
  writer.WriteUInt32(static_cast<std::uint32_t>(rooms.size()));
  for (const auto& room : rooms) {
    room.Write(writer);
  }
  return {kMessageId, writer.TakeBytes()};
}

RoomListResponse RoomListResponse::Decode(const TcpPacket& packet) {
  if (packet.message_id != kMessageId) {
    throw std::invalid_argument("Unexpected room-list response id.");
  }
  PacketReader reader(packet.payload);
  RoomListResponse response;
  response.page = reader.ReadUInt32();
  response.has_next_page = reader.ReadUInt32();
  response.value_at8 = reader.ReadUInt32();
  const auto count = reader.ReadUInt32();
  if (count > kMaxRooms ||
      count > reader.RemainingBytes() / kMinimumDescriptorBytes) {
    throw std::length_error(
        "Room list count exceeds the payload or local limit.");
  }
  response.rooms.reserve(count);
  for (std::uint32_t index = 0; index < count; ++index) {
    response.rooms.push_back(SessionDescriptor::Read(reader));
  }
  reader.RequireEnd();
  return response;
}
}  // namespace bubble_fighter::server::protocol
