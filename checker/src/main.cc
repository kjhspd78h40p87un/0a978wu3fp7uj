#include "checker_server.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "glog/logging.h"

#include <string>

#include "servers.h"

ABSL_FLAG(std::string, listen, "localhost:50053", "The address to listen on.");

int main(int argc, char **argv) {
  google::InitGoogleLogging("checker-service");
  absl::ParseCommandLine(argc, argv);
  std::string listen_address = absl::GetFlag(FLAGS_listen);
  error_specifications::RunCheckerServer(listen_address);
  google::FlushLogFiles(google::INFO);
  
  return 0;
}
