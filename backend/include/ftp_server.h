#pragma once

#include <atomic>
#include <cstdint>

#include <sockpp/tcp_acceptor.h>

namespace ftp {

class server {
public:
  server(int16_t command_port);
  ~server();

  void start();
  void stop();

private:
  void run_echo(sockpp::tcp_socket sock);
  // Todo: add a function to handle the FTP commands
  void protocol_interpreter(sockpp::tcp_socket sock);

  int16_t command_port_; // Command port (always be used)

  int16_t server_data_port_; // Data port (listening when passive mode)
  int16_t client_data_port_; // Data port (listening when active mode)

  sockpp::tcp_acceptor acceptor_;
  std::atomic<bool> running_;

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