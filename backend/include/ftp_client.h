#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include <sockpp/tcp_connector.h>

namespace ftp {

class client {
public:
  client(const std::string &server_host, int16_t server_command_port);
  ~client();

  void connect();
  void disconnect();

private:
  std::string server_host_;     // Server host
  int16_t server_command_port_; // Server command port

  int16_t server_data_port_;    // Server data port (listening when passive mode)
  int16_t client_data_port_;    // Client data port (listening when active mode)

  sockpp::tcp_connector connector_;
  std::atomic<bool> connected_;

  // Run the client from the command line
  void run_echo_from_stdin();
  // Run the protocol interpreter
  void protocol_interpreter();

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