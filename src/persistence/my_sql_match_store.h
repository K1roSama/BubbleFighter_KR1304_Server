#ifndef BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_MATCH_STORE_H_
#define BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_MATCH_STORE_H_
#include "features/rooms/room_match_settlement.h"
#include "persistence/my_sql_connection.h"

namespace bubble_fighter::server::persistence {
class MySqlMatchStore {
 public:
  explicit MySqlMatchStore(const std::filesystem::path& configuration);
  // Separate durable submission: an award transaction failure does not
  // erase the original participants, scores, policy or unique identity.
  void SavePending(const features::rooms::RoomMatchRequest& request);
  std::vector<features::rooms::RoomMatchRequest> LoadPending(
      const std::array<std::uint8_t, 16>& after = {});
  features::rooms::RoomMatchResult Settle(
      const features::rooms::RoomMatchRequest& request);

 private:
  MySqlConnection connection_;
};
}  // namespace bubble_fighter::server::persistence

#endif  // BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_MATCH_STORE_H_
