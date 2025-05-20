#pragma once

#include <atomic>
#include <cstdint>

#include <sockpp/tcp_acceptor.h>

#include "proto_interpreter.h"

namespace ftp {

class server {
public:
  server(int16_t command_port);
  ~server();

  void start();
  void stop();

private:
  void run_echo(sockpp::tcp_socket sock);

  int16_t command_port_; // Command port (always be used)

  sockpp::tcp_acceptor acceptor_;
  std::atomic<bool> running_;

  // Instances of protocol interpreter
  std::vector<protocol_interpreter_server *> interpreters_;

  // Todo: implement the FTP commands
  void do_retr();
  void do_stor();
  void do_list();
  void do_cwd();
  void do_cdup();
  void do_pwd();
  void do_mkd();
  void do_rmd();
  void do_dele();
  void do_rnfr();
  void do_rnto();
  void do_help();
  void do_quit();

  // Todo: Specify active or passive mode
  void do_port();
  void do_pasv();
};

} // namespace ftp