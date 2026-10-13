#ifndef BUBBLEFIGHTER_SERVER_APPLICATION_LOCAL_ACCOUNT_COMMAND_H_
#define BUBBLEFIGHTER_SERVER_APPLICATION_LOCAL_ACCOUNT_COMMAND_H_

namespace bubble_fighter::server::application {
class LocalAccountCommand {
 public:
  static bool IsRequested(int argc, char* argv[]);
  int Run(int argc, char* argv[]) const;
};
}  // namespace bubble_fighter::server::application

#endif  // BUBBLEFIGHTER_SERVER_APPLICATION_LOCAL_ACCOUNT_COMMAND_H_
