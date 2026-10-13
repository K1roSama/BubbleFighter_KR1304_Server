#include "application/local_account_command.h"

#include "features/authentication/account_authentication.h"
#include "persistence/my_sql_account_store.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <sddl.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace bubble_fighter::server::application {
namespace {
class PasswordInput {
 public:
  PasswordInput() {
    const auto input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD original_mode = 0;
    const bool console = GetConsoleMode(input, &original_mode) != 0;
    const auto original_code_page = GetConsoleCP();
    if (console && !SetConsoleMode(input, original_mode & ~ENABLE_ECHO_INPUT)) {
      throw std::runtime_error("Unable to disable password echo.");
    }
    struct RestoreConsole {
      HANDLE input;
      DWORD mode;
      UINT code_page;
      bool active;
      ~RestoreConsole() {
        if (active) {
          SetConsoleMode(input, mode);
          SetConsoleCP(code_page);
          std::cerr << '\n';
        }
      }
    } restore{input, original_mode, original_code_page, console};
    if (console && !SetConsoleCP(CP_UTF8)) {
      throw std::runtime_error("Unable to configure UTF-8 password input.");
    }
    std::cerr << "Password: " << std::flush;
    char value = 0;
    try {
      while (std::cin.get(value) && value != '\n') {
        if (password_.size() == 1024) {
          throw std::invalid_argument("Password exceeds the supported length.");
        }
        password_.push_back(value);
      }
      if (!password_.empty() && password_.back() == '\r') {
        password_.pop_back();
      }
      if (!password_.empty() &&
          MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, password_.data(),
                              static_cast<int>(password_.size()), nullptr,
                              0) == 0) {
        throw std::invalid_argument("Password input must be UTF-8.");
      }
    } catch (...) {
      SecureZeroMemory(password_.data(), password_.size());
      throw;
    }
  }
  ~PasswordInput() { SecureZeroMemory(password_.data(), password_.size()); }
  PasswordInput(const PasswordInput&) = delete;
  PasswordInput& operator=(const PasswordInput&) = delete;
  std::string_view Value() const { return password_; }

 private:
  std::string password_;
};

void WritePrivateTicket(const std::filesystem::path& path,
                        std::string_view ticket) {
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    throw std::runtime_error("Unable to obtain ticket file owner.");
  }
  DWORD size = 0;
  GetTokenInformation(token, TokenUser, nullptr, 0, &size);
  std::vector<std::uint8_t> information(size);
  const auto read =
      GetTokenInformation(token, TokenUser, information.data(), size, &size);
  CloseHandle(token);
  if (!read) {
    throw std::runtime_error("Unable to read ticket file owner.");
  }
  LPWSTR sid = nullptr;
  if (!ConvertSidToStringSidW(
          reinterpret_cast<TOKEN_USER*>(information.data())->User.Sid, &sid)) {
    throw std::runtime_error("Unable to format ticket file owner.");
  }
  const std::wstring sddl =
      std::wstring(L"D:P(A;;FA;;;") + sid + L")(A;;FA;;;SY)(A;;FA;;;BA)";
  LocalFree(sid);
  PSECURITY_DESCRIPTOR descriptor = nullptr;
  if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
          sddl.c_str(), SDDL_REVISION_1, &descriptor, nullptr)) {
    throw std::runtime_error("Unable to create ticket file permissions.");
  }
  SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), descriptor,
                                 FALSE};
  const auto file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, &attributes,
                                CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  LocalFree(descriptor);
  if (file == INVALID_HANDLE_VALUE) {
    throw std::runtime_error(
        "Unable to create ticket file; the path must not already exist.");
  }
  DWORD written = 0;
  const bool saved =
      WriteFile(file, ticket.data(), static_cast<DWORD>(ticket.size()),
                &written, nullptr) &&
      written == ticket.size() && FlushFileBuffers(file);
  CloseHandle(file);
  if (!saved) {
    throw std::runtime_error("Unable to finish writing ticket file.");
  }
}
}  // namespace

bool LocalAccountCommand::IsRequested(int argc, char* argv[]) {
  return argc > 1 && (std::string_view(argv[1]) == "--create-account" ||
                      std::string_view(argv[1]) == "--local-login");
}

int LocalAccountCommand::Run(int argc, char* argv[]) const {
  const bool create = std::string_view(argv[1]) == "--create-account";
  std::filesystem::path configuration;
  std::filesystem::path ticket_path;
  if (argc < 3) {
    std::cerr << "A local username is required.\n";
    return 2;
  }
  for (int index = 3; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "--database-config" && configuration.empty() &&
        index + 1 < argc) {
      configuration = argv[++index];
    } else if (argument == "--ticket-file" && !create && ticket_path.empty() &&
               index + 1 < argc) {
      ticket_path = argv[++index];
    } else {
      std::cerr << "Invalid local account command arguments.\n";
      return 2;
    }
  }
  if (configuration.empty() || (!create && ticket_path.empty())) {
    std::cerr
        << "--database-config and, for login, --ticket-file are required.\n";
    return 2;
  }
  try {
    const auto username =
        features::authentication::AccountAuthentication::NormalizeUsername(
            argv[2]);
    persistence::MySqlAccountStore store(configuration);
    features::authentication::AccountAuthentication authentication(store);
    const PasswordInput password;
    if (create) {
      if (!authentication.CreateAccount(username, password.Value())) {
        std::cerr << "Local account already exists.\n";
        return 3;
      }
      std::cout << "Local account created.\n";
    } else {
      auto ticket = authentication.Login(username, password.Value());
      if (!ticket) {
        std::cerr << "Local authentication failed.\n";
        return 3;
      }
      try {
        WritePrivateTicket(ticket_path, *ticket);
      } catch (...) {
        SecureZeroMemory(ticket->data(), ticket->size());
        throw;
      }
      SecureZeroMemory(ticket->data(), ticket->size());
      std::cout
          << "Local login succeeded; a one-use ticket was written to the requested private file.\n";
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Local account error: " << error.what() << '\n';
    return 1;
  }
}
}  // namespace bubble_fighter::server::application
