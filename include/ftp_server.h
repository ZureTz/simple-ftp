#pragma once

#include <atomic>
#include <cstdint>

#include <sockpp/tcp_acceptor.h>

#include "proto/proto_interpreter.h"

namespace ftp {

class server {
public:
  server(uint16_t command_port);
  ~server();

  void start();
  void stop();

private:
  void run_echo(sockpp::tcp_socket sock);

  uint16_t command_port_; // Command port (always be used)

  sockpp::tcp_acceptor acceptor_;
  std::atomic<bool> running_;

  // Instances of protocol interpreter
  std::vector<protocol_interpreter_server *> interpreters_;
};

} // namespace ftp