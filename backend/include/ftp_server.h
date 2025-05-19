#pragma once

#include <cstdint>
#include <atomic>
#include <sockpp/tcp_acceptor.h>

namespace ftp {
class server {
public:
  server(int16_t port);
  ~server();

  void start();
  void stop();

private:
  void run_echo(sockpp::tcp_socket sock);

  int16_t port_;
  sockpp::tcp_acceptor acceptor_;
  std::atomic<bool> running_;
};
} // namespace ftp