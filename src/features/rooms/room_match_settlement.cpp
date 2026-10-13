#include "features/rooms/room_match_settlement.h"

#include "features/characters/character_profile.h"
#define NOMINMAX
#include <Windows.h>

#include <bcrypt.h>

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace bubble_fighter::server::features::rooms {
namespace {
void WriteStatistics(protocol::PacketWriter& writer,
                     const characters::CharacterStatistics& s) {
  writer.WriteUInt32(s.level);
  writer.WriteUInt32(s.experience);
  writer.WriteUInt32(s.lucci);
  writer.WriteUInt32(s.cash);
}
characters::CharacterStatistics ReadStatistics(protocol::PacketReader& reader) {
  return {reader.ReadUInt32(), reader.ReadUInt32(), reader.ReadUInt32(),
          reader.ReadUInt32()};
}
template <std::size_t N>
void Put(const std::array<std::size_t, N>& offsets,
         std::array<std::uint32_t, N>& values, std::size_t offset,
         std::uint32_t value) {
  const auto at = std::find(offsets.begin(), offsets.end(), offset);
  if (at == offsets.end()) {
    throw std::logic_error("Unknown score field binding.");
  }
  values[static_cast<std::size_t>(at - offsets.begin())] = value;
}
}  // namespace

std::array<std::uint8_t, 16> CreateRoomMatchId() {
  std::array<std::uint8_t, 16> value{};
  if (BCryptGenRandom(nullptr, value.data(), static_cast<ULONG>(value.size()),
                      BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
    throw std::runtime_error("Cannot create a unique local match identity.");
  }
  return value;
}

void RoomMatchRequest::Validate() const {
  rules.Validate();
  if ((format_version != 1 && format_version != 2) ||
      (format_version == 1 && rules.awards != ResultAwardRules{})) {
    throw std::invalid_argument(
        "Unsupported frozen result award version or legacy rule change.");
  }
  if (std::all_of(match_id.begin(), match_id.end(),
                  [](auto v) { return v == 0; }) ||
      members.empty() || members.size() > 16 || duration_milliseconds == 0) {
    throw std::invalid_argument("Invalid local match settlement.");
  }
  std::set<std::uint64_t> accounts, characters;
  std::optional<std::uint32_t> previous;
  for (const auto& member : members) {
    if (member.slot >= 16 || (previous && *previous >= member.slot) ||
        member.account_id == 0 || member.character_id == 0 ||
        member.character_id > std::numeric_limits<std::uint32_t>::max() ||
        (member.team != 1 && member.team != 2) ||
        !accounts.insert(member.account_id).second ||
        !characters.insert(member.character_id).second) {
      throw std::invalid_argument("Invalid or duplicate match participant.");
    }
    previous = member.slot;
  }
}

std::uint8_t RoomMatchRequest::Winner() const noexcept {
  return team_scores[0] == team_scores[1]
             ? 0
             : (team_scores[0] > team_scores[1] ? 1 : 2);
}

std::vector<std::uint8_t> RoomMatchRequest::Encode() const {
  Validate();
  protocol::PacketWriter writer;
  writer.WriteUInt32(
      format_version);  // Local durable format, unrelated to client version.
  for (auto value : match_id) {
    writer.WriteUInt8(value);
  }
  for (auto value :
       {rules.experience_per_level, rules.maximum_level,
        rules.completion_experience, rules.completion_lucci,
        rules.win_experience, rules.win_lucci, rules.pang_experience,
        rules.pang_lucci, rules.rescue_experience, rules.rescue_lucci}) {
    writer.WriteUInt32(value);
  }
  if (format_version >= 2) {
    for (const auto value : rules.awards.grade_minimums) {
      writer.WriteUInt32(value);
    }
    for (const auto& row : rules.awards.medal_minimums) {
      for (const auto value : row) {
        writer.WriteUInt32(value);
      }
    }
  }
  writer.WriteUInt32(duration_milliseconds);
  for (auto value : team_scores) {
    writer.WriteUInt16(value);
  }
  writer.WriteUInt32(static_cast<std::uint32_t>(members.size()));
  for (const auto& member : members) {
    writer.WriteUInt32(member.slot);
    writer.WriteUInt64(member.account_id);
    writer.WriteUInt64(member.character_id);
    writer.WriteUInt8(member.team);
    for (auto value : member.scores) {
      writer.WriteUInt16(value);
    }
  }
  return writer.TakeBytes();
}

RoomMatchRequest RoomMatchRequest::Decode(std::span<const std::uint8_t> data) {
  if (data.size() > 4096) {
    throw std::invalid_argument("Oversized frozen match request.");
  }
  protocol::PacketReader reader(data);
  RoomMatchRequest result;
  result.format_version = reader.ReadUInt32();
  if (result.format_version != 1 && result.format_version != 2) {
    throw std::invalid_argument("Unknown frozen match version.");
  }
  for (auto& value : result.match_id) {
    value = reader.ReadUInt8();
  }
  auto& r = result.rules;
  for (auto* value : {&r.experience_per_level, &r.maximum_level,
                      &r.completion_experience, &r.completion_lucci,
                      &r.win_experience, &r.win_lucci, &r.pang_experience,
                      &r.pang_lucci, &r.rescue_experience, &r.rescue_lucci}) {
    *value = reader.ReadUInt32();
  }
  if (result.format_version >= 2) {
    for (auto& value : r.awards.grade_minimums) {
      value = reader.ReadUInt32();
    }
    for (auto& row : r.awards.medal_minimums) {
      for (auto& value : row) {
        value = reader.ReadUInt32();
      }
    }
  }
  result.duration_milliseconds = reader.ReadUInt32();
  for (auto& value : result.team_scores) {
    value = reader.ReadUInt16();
  }
  const auto count = reader.ReadUInt32();
  if (count == 0 || count > 16 || reader.RemainingBytes() != count * 29) {
    throw std::invalid_argument("Invalid frozen match member count.");
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    RoomMatchMember member;
    member.slot = reader.ReadUInt32();
    member.account_id = reader.ReadUInt64();
    member.character_id = reader.ReadUInt64();
    member.team = reader.ReadUInt8();
    for (auto& value : member.scores) {
      value = reader.ReadUInt16();
    }
    result.members.push_back(member);
  }
  reader.RequireEnd();
  result.Validate();
  return result;
}

void RoomMatchResult::Validate(const RoomMatchRequest& request) const {
  request.Validate();
  if (rewards.size() != request.members.size()) {
    throw std::invalid_argument("Incomplete match receipt.");
  }
  for (std::size_t i = 0; i < rewards.size(); ++i) {
    const auto& reward = rewards[i];
    const auto& member = request.members[i];
    if (reward.slot != member.slot ||
        reward.character.character_id != member.character_id ||
        reward.character.account_id != member.account_id ||
        reward.character.statistics !=
            request.rules.Apply(reward.before, member.scores,
                                member.team == request.Winner())) {
      throw std::invalid_argument(
          "Match receipt does not match trusted award inputs.");
    }
    characters::CharacterProfile::BindKnownFields({}, reward.character,
                                                  member.account_id);
  }
}

std::vector<std::uint8_t> RoomMatchResult::Encode() const {
  if (rewards.empty() || rewards.size() > 16) {
    throw std::invalid_argument("Invalid local receipt size.");
  }
  protocol::PacketWriter writer;
  writer.WriteUInt32(1);
  writer.WriteUInt32(static_cast<std::uint32_t>(rewards.size()));
  for (const auto& reward : rewards) {
    const auto& c = reward.character;
    writer.WriteUInt32(reward.slot);
    writer.WriteUInt64(c.character_id);
    writer.WriteUInt64(c.account_id);
    writer.WriteUtf16(c.character_name);
    writer.WriteUInt32(c.character_type);
    writer.WriteUInt32(c.color_index);
    WriteStatistics(writer, reward.before);
    WriteStatistics(writer, c.statistics);
  }
  return writer.TakeBytes();
}

RoomMatchResult RoomMatchResult::Decode(std::span<const std::uint8_t> data) {
  if (data.size() > 4096) {
    throw std::invalid_argument("Oversized local match receipt.");
  }
  protocol::PacketReader reader(data);
  if (reader.ReadUInt32() != 1) {
    throw std::invalid_argument("Unknown local match receipt version.");
  }
  const auto count = reader.ReadUInt32();
  if (count == 0 || count > 16) {
    throw std::invalid_argument("Invalid local receipt count.");
  }
  RoomMatchResult result;
  for (std::uint32_t i = 0; i < count; ++i) {
    RoomMatchReward reward;
    reward.slot = reader.ReadUInt32();
    auto& c = reward.character;
    c.character_id = reader.ReadUInt64();
    c.account_id = reader.ReadUInt64();
    c.character_name = reader.ReadUtf16();
    c.character_type = reader.ReadUInt32();
    c.color_index = reader.ReadUInt32();
    reward.before = ReadStatistics(reader);
    c.statistics = ReadStatistics(reader);
    result.rewards.push_back(std::move(reward));
  }
  reader.RequireEnd();
  return result;
}

protocol::GameScoreNotification RoomMatchResult::Scores(
    const RoomMatchRequest& request) const {
  Validate(request);
  protocol::GameScoreNotification result;
  result.elapsed_milliseconds = request.duration_milliseconds;
  auto& scores = result.scores;
  scores.byte_at452 = request.Winner();  // 42A340: 0 draw, 1/2 explicit team.
  scores.triples.resize(
      36);  // Native constructor capacity; third column native zero table.
  scores.triples[0][0] = request.team_scores[0];
  scores.triples[1][0] = request.team_scores[1];
  for (std::size_t i = 0; i < rewards.size(); ++i) {
    const auto& reward = rewards[i];
    const auto& member = request.members[i];
    const auto& c = reward.character;
    const auto& s = c.statistics;
    protocol::GameScorePlayerRecord p;
    p.value_at500 = 1;
    p.value_at0 = static_cast<std::uint32_t>(c.character_id);
    p.text_at4 = c.character_name;
    p.byte_at52 = member.team;
    const auto put = [&](std::size_t offset, std::uint32_t value) {
      Put(p.kBeforeTextOffsets, p.values_before_texts, offset, value);
    };
    put(56, s.level);
    put(60, member.scores[0]);
    put(64, member.scores[1]);
    put(80, member.scores[2]);
    const auto award = request.rules.awards.Evaluate(member.scores);
    put(84,
        request.format_version >= 2 ? award.contribution : member.scores[1]);
    const auto range = request.rules.ExperienceRange(s.experience);
    put(368, range[0]);
    put(372, s.experience);
    put(376, range[1]);
    put(380, s.experience - reward.before.experience);
    put(384, s.lucci - reward.before.lucci);
    // +388 remains native reset false: its special animation has not
    // been established. S105 carries the committed level explicitly.
    Put(p.kAfterTextOffsets, p.values_after_texts, 236, member.scores[3]);
    // C48D20 reads +392 as Rank_list frame; it is not the medal ID.
    Put(p.kAfterTextOffsets, p.values_after_texts, 392,
        request.format_version >= 2 ? award.grade : 0xFFFFFFFFU);
    p.value_in_second_tail = 0xFFFFFFFFU;
    if (request.format_version >= 2 && award.medal != 0) {
      // C70280: relative medal fallback and explicit BF_title mode*10000+ID.
      p.value_in_first_tail = award.medal;
      p.value_in_second_tail = 10000U + award.medal;
    }
    scores.players.emplace(static_cast<std::uint8_t>(member.slot),
                           std::move(p));
  }
  return result;
}

std::vector<std::pair<std::uint32_t, protocol::TcpPacket>>
RoomMatchResult::Profiles(const RoomMatchRequest& request) const {
  Validate(request);
  std::vector<std::pair<std::uint32_t, protocol::TcpPacket>> result;
  for (const auto& reward : rewards) {
    auto profile = characters::CreateCharacterProfileBaseline();
    profile = characters::CharacterProfile::BindKnownFields(
        std::move(profile), reward.character, reward.character.account_id);
    const auto range =
        request.rules.ExperienceRange(reward.character.statistics.experience);
    profile.values[2] = range[0];
    profile.values[3] = range[1];
    result.emplace_back(reward.slot, profile.Encode());
  }
  return result;
}
}  // namespace bubble_fighter::server::features::rooms
