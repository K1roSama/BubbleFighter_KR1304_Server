#include "persistence/my_sql_connection.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <mysql.h>

#include <charconv>
#include <fstream>
#include <map>
#include <string>

namespace bubble_fighter::server::persistence {
namespace {
void InitializeRuntime() {
  struct Library {
    Library() {
      if (mysql_library_init(0, nullptr, nullptr) != 0) {
        throw MySqlError(0);
      }
    }
    ~Library() { mysql_library_end(); }
  };
  static Library library;
  struct Thread {
    Thread() {
      if (mysql_thread_init() != 0) {
        throw MySqlError(0);
      }
    }
    ~Thread() { mysql_thread_end(); }
  };
  thread_local Thread thread;
}

std::string Trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r");
  if (first == std::string::npos) {
    return {};
  }
  return value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
}

struct Options {
  std::string host;
  std::string user;
  std::string password;
  std::string database;
  unsigned int port = 0;
  ~Options() {
    if (!password.empty()) {
      SecureZeroMemory(password.data(), password.size());
    }
  }
};

void LoadOptions(const std::filesystem::path& path, Options& options) {
  if (!std::filesystem::is_regular_file(path) ||
      std::filesystem::file_size(path) > 16384) {
    throw std::invalid_argument(
        "Missing or oversized database configuration file.");
  }
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Unable to read database configuration.");
  }
  std::map<std::string, std::string> values;
  std::string section;
  std::string line;
  while (std::getline(file, line)) {
    line = Trim(std::move(line));
    if (line.empty() || line.front() == '#' || line.front() == ';') {
      continue;
    }
    if (line.front() == '[' && line.back() == ']') {
      section = line;
      if (section != "[client]" && section != "[mysql]") {
        throw std::invalid_argument(
            "Unsupported database configuration section.");
      }
      continue;
    }
    const auto separator = line.find('=');
    if (section.empty() || separator == std::string::npos) {
      throw std::invalid_argument("Malformed database configuration.");
    }
    const auto key = Trim(line.substr(0, separator));
    auto value = Trim(line.substr(separator + 1));
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
      value = value.substr(1, value.size() - 2);
    }
    if (key != "host" && key != "port" && key != "protocol" && key != "user" &&
        key != "password" && key != "ssl-mode" && key != "database") {
      throw std::invalid_argument("Unsupported database configuration key.");
    }
    if (value.find('\0') != std::string::npos ||
        !values.emplace(key, std::move(value)).second) {
      throw std::invalid_argument(
          "Invalid or duplicate database configuration value.");
    }
    SecureZeroMemory(line.data(), line.size());
  }
  // This development connector deliberately supports only loopback TCP
  // with required TLS. Remote certificate configuration is not implied.
  if (values["host"] != "127.0.0.1" || values["protocol"] != "TCP" ||
      values["ssl-mode"] != "REQUIRED" || values["user"].empty() ||
      values["password"].empty()) {
    throw std::invalid_argument(
        "Local database requires explicit loopback TCP, credentials and TLS.");
  }
  const auto& port_text = values["port"];
  const auto parsed = std::from_chars(
      port_text.data(), port_text.data() + port_text.size(), options.port);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != port_text.data() + port_text.size() || options.port == 0 ||
      options.port > 65535) {
    throw std::invalid_argument("Invalid local database port.");
  }
  options.host = std::move(values["host"]);
  options.user = std::move(values["user"]);
  options.password = std::move(values["password"]);
  options.database = values.contains("database") ? std::move(values["database"])
                                                 : "BubbleFighterServer";
  if (options.database.empty() || options.database.size() > 64) {
    throw std::invalid_argument("Invalid local database name.");
  }
}
}  // namespace

struct MySqlConnection::Impl {
  MYSQL* connection = nullptr;
  ~Impl() {
    if (connection) {
      mysql_close(connection);
    }
  }
};

MySqlError::MySqlError(unsigned int code)
    : std::runtime_error("MySQL operation failed (code " +
                         std::to_string(code) + ")."),
      code_(code) {}
unsigned int MySqlError::Code() const { return code_; }

MySqlConnection::MySqlConnection(
    const std::filesystem::path& configuration_file)
    : impl_(std::make_unique<Impl>()) {
  Options options;
  LoadOptions(configuration_file, options);
  InitializeRuntime();
  impl_->connection = mysql_init(nullptr);
  auto* connection = impl_->connection;
  if (!connection) {
    throw MySqlError(0);
  }
  const auto set_option = [connection](mysql_option option, const void* value) {
    if (mysql_options(connection, option, value) != 0) {
      throw MySqlError(mysql_errno(connection));
    }
  };
  const unsigned int timeout = 5;
  const unsigned int local_files = 0;
  const unsigned int protocol = MYSQL_PROTOCOL_TCP;
  const unsigned int ssl = SSL_MODE_REQUIRED;
  set_option(MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
  set_option(MYSQL_OPT_READ_TIMEOUT, &timeout);
  set_option(MYSQL_OPT_WRITE_TIMEOUT, &timeout);
  set_option(MYSQL_OPT_LOCAL_INFILE, &local_files);
  set_option(MYSQL_OPT_PROTOCOL, &protocol);
  set_option(MYSQL_OPT_SSL_MODE, &ssl);
  set_option(MYSQL_SET_CHARSET_NAME, "utf8mb4");
  if (!mysql_real_connect(connection, options.host.c_str(),
                          options.user.c_str(), options.password.c_str(),
                          options.database.c_str(), options.port, nullptr, 0)) {
    throw MySqlError(mysql_errno(connection));
  }
  if (!mysql_get_ssl_cipher(connection)) {
    throw std::runtime_error("Database connection did not negotiate TLS.");
  }
  if (mysql_query(connection, "SET time_zone = '+00:00'") != 0) {
    throw MySqlError(mysql_errno(connection));
  }
}

MySqlConnection::~MySqlConnection() = default;
MYSQL* MySqlConnection::Handle() const { return impl_->connection; }
}  // namespace bubble_fighter::server::persistence
