#ifndef BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_COMMANDS_H_
#define BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_COMMANDS_H_

#include <mysql.h>

#include <cstdint>
#include <span>
#include <string_view>

#include "persistence/my_sql_connection.h"

namespace bubble_fighter::server::persistence::my_sql_commands {
class Statement {
 public:
  Statement(MYSQL* connection, std::string_view sql)
      : statement_(mysql_stmt_init(connection)) {
    if (!statement_) {
      throw MySqlError(mysql_errno(connection));
    }
    if (mysql_stmt_prepare(statement_, sql.data(),
                           static_cast<unsigned long>(sql.size())) != 0) {
      const auto code = mysql_stmt_errno(statement_);
      mysql_stmt_close(statement_);
      throw MySqlError(code);
    }
  }
  ~Statement() { mysql_stmt_close(statement_); }
  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;
  void Execute(std::span<MYSQL_BIND> parameters) {
    if (mysql_stmt_param_count(statement_) != parameters.size()) {
      throw std::logic_error("SQL parameter count mismatch.");
    }
    if (mysql_stmt_bind_param(statement_, parameters.data()) != 0 ||
        mysql_stmt_execute(statement_) != 0) {
      throw MySqlError(mysql_stmt_errno(statement_));
    }
  }
  bool Fetch(std::span<MYSQL_BIND> results) {
    if (mysql_stmt_field_count(statement_) != results.size()) {
      throw std::runtime_error("Unexpected database result shape.");
    }
    if (mysql_stmt_bind_result(statement_, results.data()) != 0) {
      throw MySqlError(mysql_stmt_errno(statement_));
    }
    const auto result = mysql_stmt_fetch(statement_);
    if (result == MYSQL_NO_DATA) {
      return false;
    }
    if (result != 0) {
      throw MySqlError(mysql_stmt_errno(statement_));
    }
    return true;
  }
  std::uint64_t AffectedRows() const {
    return mysql_stmt_affected_rows(statement_);
  }

 private:
  MYSQL_STMT* statement_;
};

inline MYSQL_BIND Bytes(const void* data, unsigned long size,
                        enum_field_types type = MYSQL_TYPE_BLOB) {
  MYSQL_BIND binding{};
  binding.buffer_type = type;
  binding.buffer = const_cast<void*>(data);
  binding.buffer_length = size;
  return binding;
}

template <typename Integer>
MYSQL_BIND Number(const Integer& value) {
  static_assert(sizeof(Integer) == 1 || sizeof(Integer) == 4 ||
                sizeof(Integer) == 8);
  MYSQL_BIND binding{};
  binding.buffer_type =
      sizeof(Integer) == 8
          ? MYSQL_TYPE_LONGLONG
          : (sizeof(Integer) == 4 ? MYSQL_TYPE_LONG : MYSQL_TYPE_TINY);
  binding.buffer = const_cast<Integer*>(&value);
  binding.is_unsigned = true;
  return binding;
}

class Transaction {
 public:
  explicit Transaction(MYSQL* connection) : connection_(connection) {
    if (mysql_autocommit(connection_, false) != 0) {
      throw MySqlError(mysql_errno(connection_));
    }
  }
  ~Transaction() {
    if (active_) {
      mysql_rollback(connection_);
      mysql_autocommit(connection_, true);
    }
  }
  void Commit() {
    if (mysql_commit(connection_) != 0 ||
        mysql_autocommit(connection_, true) != 0) {
      throw MySqlError(mysql_errno(connection_));
    }
    active_ = false;
  }

 private:
  MYSQL* connection_;
  bool active_ = true;
};
}  // namespace bubble_fighter::server::persistence::my_sql_commands

#endif  // BUBBLEFIGHTER_SERVER_PERSISTENCE_MY_SQL_COMMANDS_H_
