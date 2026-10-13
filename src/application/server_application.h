#ifndef BUBBLEFIGHTER_SERVER_APPLICATION_SERVER_APPLICATION_H_
#define BUBBLEFIGHTER_SERVER_APPLICATION_SERVER_APPLICATION_H_

namespace bubble_fighter::server::application {
class ServerApplication {
 public:
  int Run(int argc, char* argv[]) const;
};
}  // namespace bubble_fighter::server::application

#endif  // BUBBLEFIGHTER_SERVER_APPLICATION_SERVER_APPLICATION_H_
