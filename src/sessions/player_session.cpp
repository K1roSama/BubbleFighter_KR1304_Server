#include "sessions/player_session.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "features/authentication/initial_handshake.h"
#include "features/authentication/login_profile.h"
#include "features/characters/character_creation.h"
#include "features/characters/character_store.h"
#include "features/configuration/content_config_response.h"
#include "features/lobby/plaza_registry.h"
#include "features/parties/solo_party.h"
#include "features/rooms/room_registry.h"
#include "protocol/characters/check_create_user_name_request.h"
#include "protocol/characters/check_create_user_name_response.h"
#include "protocol/characters/create_user_info_notification.h"
#include "protocol/characters/create_user_info_request.h"
#include "protocol/characters/create_user_info_response.h"
#include "protocol/codec/tcp_packet_codec.h"
#include "protocol/configuration/dynamic_config_request.h"
#include "protocol/inventory/equip_item_request.h"
#include "protocol/inventory/item_use_request.h"
#include "protocol/inventory/unequip_item_request.h"
#include "protocol/inventory/weapon_preset_change_request.h"
#include "protocol/login/login_request.h"
#include "protocol/scenes/chat_request.h"
#include "protocol/scenes/plaza_movement.h"
#include "protocol/unresolved/client_to_server_message_0x0021.h"
#include "protocol/unresolved/client_to_server_message_0x0069.h"
#include "protocol/unresolved/client_to_server_message_0x006a.h"
#include "protocol/unresolved/client_to_server_message_0x0209.h"
#include "sessions/menu_initialization.h"

namespace bubble_fighter::server::sessions {
std::uint64_t PlayerSession::InventoryObservationTime() {
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
  if (seconds < 0) {
    throw std::runtime_error("Server clock predates the Unix epoch.");
  }
  return static_cast<std::uint64_t>(seconds);
}

PlayerSession::PlayerSession(
    TicketConsumer consume_ticket,
    std::shared_ptr<features::characters::CharacterStore> characters,
    MenuDataProvider menu_data, EquipmentChangeHandler equipment_change,
    ShopCatalogProvider shop_catalog, ShopPurchaseHandler shop_purchase,
    CashPickupHandler cash_pickup, WeaponPresetHandler weapon_preset,
    ItemUseHandler item_use, PracticeClock practice_clock,
    PracticeSeedFactory practice_seed,
    std::shared_ptr<features::rooms::RoomRegistry> rooms,
    std::shared_ptr<features::lobby::PlazaRegistry> plazas)
    : consume_ticket_(std::move(consume_ticket)),
      characters_(std::move(characters)),
      menu_data_(std::move(menu_data)),
      equipment_change_(std::move(equipment_change)),
      shop_catalog_(std::move(shop_catalog)),
      shop_purchase_(std::move(shop_purchase)),
      cash_pickup_(std::move(cash_pickup)),
      weapon_preset_(std::move(weapon_preset)),
      item_use_(std::move(item_use)),
      practice_clock_(std::move(practice_clock)),
      practice_seed_(std::move(practice_seed)),
      rooms_(std::move(rooms)),
      plazas_(std::move(plazas)) {
  if (!practice_clock_ || !practice_seed_) {
    throw std::invalid_argument(
        "Missing practice clock or random seed source.");
  }
}

std::vector<protocol::TcpPacket> PlayerSession::Process(
    const protocol::TcpPacket& packet) {
  try {
    if (room_) {
      room_.Poll();
      if (auto current = room_.Character()) {
        character_ = std::move(current);
      }
    }
    auto result = plaza_packets_->Drain();
    for (auto& response : room_packets_->Drain()) {
      result.push_back(std::move(response));
    }
    auto direct = ProcessCore(packet);
    if (plaza_ &&
        (packet.message_id == protocol::EquipItemRequest::kMessageId ||
         packet.message_id == protocol::UnequipItemRequest::kMessageId ||
         packet.message_id == protocol::WeaponPresetChangeRequest::kMessageId ||
         packet.message_id == protocol::ItemUseRequest::kMessageId)) {
      auto data = menu_data_(*character_, InventoryObservationTime());
      plaza_.Refresh(
          {std::move(data.member), *character_, std::move(data.equipment)},
          false);
    }
    for (auto& response : direct) {
      result.push_back(std::move(response));
    }
    for (auto& response : room_packets_->Drain()) {
      result.push_back(std::move(response));
    }
    for (auto& response : plaza_packets_->Drain()) {
      result.push_back(std::move(response));
    }
    return result;
  } catch (...) {
    Reject();
    throw;
  }
}

std::vector<protocol::TcpPacket> PlayerSession::ProcessCore(
    const protocol::TcpPacket& packet) {
  using namespace protocol;
  try {
    if (state_ == PlayerSessionState::kRejected) {
      throw std::runtime_error("Game login session rejected.");
    }
    if (auto responses = ProcessRoom(packet)) {
      return std::move(*responses);
    }
    if (packet.message_id == ChatRequest::kMessageId) {
      const auto request = ChatRequest::Decode(packet);
      if (!account_id_ || !character_ || !menu_data_ || !lobby_ ||
          (!CommerceInitialized() && !practice_)) {
        throw std::runtime_error("Chat before authenticated initialization.");
      }
      if (!chat_.Admit(practice_clock_())) {
        return {};
      }
      if (!features::chat::SceneChat::ValidText(request.text)) {
        return {features::chat::SceneChat::Notice(
            u"聊天内容须为 1 至 256 个有效字符。")};
      }
      if (request.channel > 1) {
        return {features::chat::SceneChat::Notice(
            u"该聊天频道需要对应的社交系统，当前尚未开放。")};
      }
      auto message = features::chat::SceneChat::Prepare(request, *character_);
      if (!request.expression.empty() &&
          features::chat::SceneChat::NeedsInventory(request.text)) {
        const auto observed_at = InventoryObservationTime();
        const auto data = menu_data_(*character_, observed_at);
        message = features::chat::SceneChat::Prepare(
            request, *character_, &data.inventory, observed_at);
      }
      if (room_) {
        room_.Chat(message);
        return {};
      }
      if (plaza_) {
        plaza_.Chat(message);
        return {};
      }
      if (request.channel == 1) {
        return {features::chat::SceneChat::Notice(
            u"队伍聊天仅在房间或对战中可用。")};
      }
      return {features::chat::SceneChat::Create(-3, character_->character_name,
                                                message)
                  .Encode()};
    }
    if (packet.message_id == PlazaMovementRequest::kMessageId) {
      const auto movement = PlazaMovementRequest::Decode(packet);
      if (plaza_) {
        plaza_.Move(movement);
      }
      return {};  // Late movement during a scene transition has no authority.
    }
    if (packet.message_id == DynamicConfigRequest::kMessageId) {
      if (state_ == PlayerSessionState::kAwaitingInitialRecord) {
        throw std::runtime_error(
            "Dynamic configuration before initial record.");
      }
      return {features::configuration::RespondContentConfiguration(
                  DynamicConfigRequest::Decode(packet))
                  .Encode()};
    }
    if (packet.message_id == LoginRequest::kMessageId) {
      if (state_ != PlayerSessionState::kAwaitingTicket) {
        throw std::runtime_error("Login request in invalid session state.");
      }
      if (!consume_ticket_ || !characters_) {
        throw std::runtime_error(
            "Local game authentication is not configured.");
      }
      const auto request = LoginRequest::Decode(packet);
      if (request.nexon_passport.size() != 69) {
        throw std::runtime_error("Invalid local ticket.");
      }
      std::string ticket;
      for (const auto code_unit : request.nexon_passport) {
        if (code_unit > 127) {
          throw std::runtime_error("Invalid local ticket.");
        }
        ticket.push_back(static_cast<char>(code_unit));
      }
      const auto account = consume_ticket_(ticket);
      if (!account || *account == 0) {
        throw std::runtime_error("Local ticket rejected.");
      }
      account_id_ = account;
      character_ = characters_->FindByAccount(*account_id_);
      if (character_) {
        state_ = PlayerSessionState::kAwaitingCharacterProfile;
        return SendLoginProfile();
      }
      state_ = PlayerSessionState::kAwaitingCharacterCreation;
      return {CreateUserInfoNotification{}.Encode()};
    }
    if (packet.message_id == CheckCreateUserNameRequest::kMessageId ||
        packet.message_id == CreateUserInfoRequest::kMessageId) {
      if (!account_id_ || !characters_) {
        throw std::runtime_error("Character operation before authentication.");
      }
      if (state_ != PlayerSessionState::kAwaitingCharacterCreation) {
        throw std::runtime_error(
            "Character operation in invalid session state.");
      }
      features::characters::CharacterCreation creation(*characters_);
      if (packet.message_id == CheckCreateUserNameRequest::kMessageId) {
        const auto request = CheckCreateUserNameRequest::Decode(packet);
        const auto result = creation.CheckName(request.character_name);
        checked_name_ = result == 0 ? request.character_name : std::u16string{};
        return {CheckCreateUserNameResponse{result, request.character_name}
                    .Encode()};
      }
      const auto request = CreateUserInfoRequest::Decode(packet);
      if (checked_name_.empty() || checked_name_ != request.character_name) {
        return {CreateUserInfoResponse{2}.Encode()};
      }
      const auto result = creation.Create(*account_id_, request);
      checked_name_.clear();
      if (result == 0) {
        character_ = characters_->FindByAccount(*account_id_);
        if (!character_) {
          throw std::runtime_error("Created character was not persisted.");
        }
        state_ = PlayerSessionState::kAwaitingCharacterProfile;
      }
      return {CreateUserInfoResponse{result}.Encode()};
    }
    if (packet.message_id == ClientToServerMessage0x0021::kMessageId) {
      if (!account_id_) {
        throw std::runtime_error("Creation follow-up before authentication.");
      }
      ClientToServerMessage0x0021::Decode(packet);
      if (state_ == PlayerSessionState::kAwaitingCharacterProfile) {
        return SendLoginProfile();
      }
      // E028: this is also reachable after failed creation. It cannot
      // grant identity or imply a successful persisted character.
      return {};
    }
    if (packet.message_id == ClientToServerMessage0x0209::kMessageId) {
      ClientToServerMessage0x0209::Decode(packet);
      if (!account_id_ || !character_ ||
          (state_ != PlayerSessionState::kLoginProfileSent &&
           state_ != PlayerSessionState::kAwaitingMenuInitialization &&
           state_ != PlayerSessionState::kMenuInitializationSent &&
           state_ != PlayerSessionState::kPracticeLoading &&
           state_ != PlayerSessionState::kPracticeCountdown &&
           state_ != PlayerSessionState::kPracticePlaying &&
           state_ != PlayerSessionState::kPracticeAwaitingReport &&
           state_ != PlayerSessionState::kPracticeFinishing &&
           state_ != PlayerSessionState::kPracticeResult &&
           state_ != PlayerSessionState::kInRoom)) {
        throw std::runtime_error(
            "C521 before authenticated character profile.");
      }
      if (state_ != PlayerSessionState::kAwaitingMenuInitialization) {
        return {};
      }
      // E054 local policy: first observed common-UI initialization
      // signal after S3. This does not assign C521 a recovered request
      // meaning. Repeated UI entry cannot reload/reset menu state.
      const auto observed_at = InventoryObservationTime();
      auto responses = MenuInitialization::Prepare(
          *character_, *account_id_, menu_data_(*character_, observed_at),
          observed_at);
      state_ = PlayerSessionState::kMenuInitializationSent;
      return responses;
    }
    if (auto responses = ProcessPractice(packet)) {
      return std::move(*responses);
    }
    if (packet.message_id == ClientToServerMessage0x006A::kMessageId) {
      if ((state_ != PlayerSessionState::kLoginProfileSent &&
           state_ != PlayerSessionState::kMenuInitializationSent &&
           !practice_) ||
          !account_id_ || !character_ || !lobby_) {
        throw std::runtime_error(
            "Lobby request before authenticated character initialization.");
      }
      ClientToServerMessage0x006A::Decode(packet);
      if (menu_data_) {
        // Reload trusted local state; do not carry a partially
        // initialized menu context into the plaza. S117 contains
        // both equipment maps and the owned local member.
        const auto observed_at = InventoryObservationTime();
        auto data = menu_data_(*character_, observed_at);
        auto packets = EnterPlaza(std::move(data));
        practice_.reset();  // S117 type 3 replaces the game model and selects
                            // LobbyStage.
        state_ = PlayerSessionState::kMenuInitializationSent;
        return packets;
      }
      return {lobby_->RequestEntry({}).Encode()};
    }
    if (packet.message_id == ClientToServerMessage0x0069::kMessageId) {
      ClientToServerMessage0x0069::Decode(packet);
      if (state_ != PlayerSessionState::kMenuInitializationSent ||
          !menu_data_ || !account_id_ || !character_ || !lobby_) {
        throw std::runtime_error(
            "Menu return before full authenticated initialization.");
      }
      // Local menu/plaza policy: return with an owned fresh snapshot.
      // S117 type 1 establishes the context before selecting stage 2.
      // Room, party and active-match leave semantics are not assigned
      // to this branch until those server states are implemented.
      const auto observed_at = InventoryObservationTime();
      auto responses = MenuInitialization::Prepare(
          *character_, *account_id_, menu_data_(*character_, observed_at),
          observed_at);
      plaza_.Release();
      plaza_packets_->Clear();
      lobby_.emplace();
      return responses;
    }
    if (auto responses = ProcessCommerce(packet)) {
      return std::move(*responses);
    }
    auto responses = handshake_.Process(packet);
    if (handshake_.State() ==
        features::authentication::InitialHandshakeState::kRejected) {
      state_ = PlayerSessionState::kRejected;
    } else if (state_ == PlayerSessionState::kAwaitingInitialRecord &&
               handshake_.State() ==
                   features::authentication::InitialHandshakeState::
                       kInitialRecordAccepted) {
      state_ = PlayerSessionState::kAwaitingTicket;
    }
    return responses;
  } catch (...) {
    Reject();
    throw;
  }
}

std::vector<protocol::TcpPacket> PlayerSession::Poll() {
  try {
    if (state_ == PlayerSessionState::kRejected) {
      throw std::runtime_error("Game login session rejected.");
    }
    if (room_) {
      room_.Poll();
      if (auto current = room_.Character()) {
        character_ = std::move(current);
      }
      return room_packets_->Drain();
    }
    if (!practice_) {
      return plaza_packets_->Drain();
    }
    const auto now = practice_clock_();
    if (practice_->Expired(now)) {
      return ReturnFromPractice();
    }
    auto packets = practice_->Poll(now);
    UpdatePracticeState();
    return packets;
  } catch (...) {
    Reject();
    throw;
  }
}

void PlayerSession::Reject() noexcept {
  room_.Release();
  plaza_.Release();
  state_ = PlayerSessionState::kRejected;
  account_id_.reset();
  character_.reset();
  checked_name_.clear();
  lobby_.reset();
  shop_revision_sent_.reset();
  practice_.reset();
}

std::vector<protocol::TcpPacket> PlayerSession::SendLoginProfile() {
  if (!account_id_ || !character_) {
    throw std::runtime_error("Missing authenticated character profile.");
  }
  auto packet = features::authentication::LoginProfile::Create(
                    *account_id_, *character_, std::chrono::system_clock::now())
                    .Encode();
  auto party = features::parties::SoloParty::Create(*character_).Encode();
  lobby_.emplace();
  state_ = menu_data_ ? PlayerSessionState::kAwaitingMenuInitialization
                      : PlayerSessionState::kLoginProfileSent;
  return {std::move(packet), std::move(party)};
}

PlayerSessionState PlayerSession::State() const noexcept { return state_; }
bool PlayerSession::CommerceInitialized() const noexcept {
  return state_ == PlayerSessionState::kMenuInitializationSent ||
         (state_ == PlayerSessionState::kInRoom && room_);
}
std::vector<protocol::TcpPacket> PlayerSession::ChangeInventory(
    const std::function<std::vector<protocol::TcpPacket>()>& change) {
  if (!room_) {
    return change();
  }
  std::vector<protocol::TcpPacket> result;
  if (!room_.ChangeInventory([&] {
        result = change();
        auto data = menu_data_(*character_, InventoryObservationTime());
        return features::rooms::RoomInventoryChange{*character_,
                                                    std::move(data.equipment)};
      })) {
    return {features::chat::SceneChat::Notice(
        u"对战进行中，请返回等待房间后更换装备或使用道具。")};
  }
  return result;
}
std::optional<std::uint64_t> PlayerSession::AccountId() const noexcept {
  return account_id_;
}
const std::optional<features::characters::CharacterRecord>&
PlayerSession::Character() const noexcept {
  return character_;
}
}  // namespace bubble_fighter::server::sessions
