#include "application/server_application.h"

int main(int argc, char* argv[]) {
  const bubble_fighter::server::application::ServerApplication application;
  return application.Run(argc, argv);
}
