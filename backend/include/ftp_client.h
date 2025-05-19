#pragma once

#include <atomic>
#include <string>

#include <sockpp/tcp_connector.h>

namespace ftp {

class client {
public:
  client(const std::string &host, int16_t port);
  ~client();

  void connect();
  void disconnect();
  // Run the client from the command line
  void run_echo_from_stdin();

private:
  std::string host_;
  int16_t port_;
  sockpp::tcp_connector connector_;
  std::atomic<bool> connected_;
};

} // namespace ftp