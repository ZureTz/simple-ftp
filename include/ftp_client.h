#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include <sockpp/tcp_connector.h>
#include <sockpp/tcp_socket.h>

#include "proto/proto_interpreter.h"

namespace ftp {

class client {
public:
  client(const std::string &server_host, int16_t server_command_port);
  ~client() = default;

  void connect();
  void disconnect();

private:
  std::string server_host_;     // Server host
  int16_t server_command_port_; // Server command port

  sockpp::tcp_connector connector_;
  std::atomic<bool> connected_;

  // Run the client from the command line
  void run_echo_from_stdin();

  // Protocol interpreter (single instance)
  protocol_interpreter_client *protocol_interpreter_;
};

} // namespace ftp