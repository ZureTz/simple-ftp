#pragma once

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <sockpp/tcp_acceptor.h>
#include <string>

#include <sockpp/tcp_connector.h>
#include <sockpp/tcp_socket.h>

namespace ftp {

class protocol_interpreter_client {
public:
  protocol_interpreter_client(sockpp::tcp_connector *const connector);
  ~protocol_interpreter_client() = default;

  void run();
  void stop();

  // Is protocol interpreter running?
  bool is_running() const;

private:
  sockpp::tcp_connector *connector_;
  std::atomic<bool> running_;

  // Buffer for reading data from the server
  std::shared_ptr<char> buf_;

  // States of the protocol interpreter
  // 0: Not logged in
  // 1: Logged in
  // 2: Handshake passive mode or active mode
  // 3: with Passive mode | Active mode already established:
  //      | Establish data connection based on the mode
  //      | File transfer mode
  //      | File transfer done
  //    loop until the user quits

  // Default to passive mode (client may be behind a NAT)
  bool is_passive_mode_;

  // Client listening port in active mode
  uint16_t client_data_port_;

  // Send username to the server, wait for response
  void do_user(std::string username);
  // Send password to the server, wait for response
  void do_pass(std::string password);

  // Todo: Specify active or passive mode
  // Send PORT command to the server, wait for response
  void do_port(std::string port);
  // Send PASV command to the server, wait for response
  void do_pasv();

  // Todo: implement the FTP commands
  // Retrieve file from the server, save it to the local file system
  // And wait for response
  void do_retr(std::string filename);
  // Store file to the server, read it from the local file system
  // And wait for response
  void do_stor(std::string filename);
  // List files in the current directory, wait for response
  void do_list();
  // Change working directory, wait for response
  void do_cwd(std::string directory);
  // Change to parent directory, wait for response
  void do_cdup(std::string directory);
  // Print working directory, wait for response
  void do_pwd();
  // Make directory, wait for response
  void do_mkd(std::string directory);
  // Remove directory, wait for response
  void do_rmd(std::string directory);
  // Delete file, wait for response
  void do_dele(std::string filename);
  // Rename from, wait for response
  void do_rnfr(std::string oldname);
  // Rename to, wait for response
  void do_rnto(std::string newname);

  // Help command, runs locally without server
  void do_help();

  // send_file() and recv_file() are used to send and receive files over a
  // socket.
  // These functions will establish a data connection with the client
  // based on the mode (active or passive)
  void send_file(std::string filename);
  void receive_file(std::string filename);

  // Implementation of file() and receive_file() in active mode and
  // passive mode
  void send_file_active(std::string filename);
  void send_file_passive(std::string filename);

  void receive_file_active(std::string filename);
  void receive_file_passive(std::string filename);
};

class protocol_interpreter_server {
public:
  protocol_interpreter_server(sockpp::tcp_socket sock);
  ~protocol_interpreter_server() = default;

  void run();
  void stop();

  // Is protocol interpreter running?
  bool is_running() const;

private:
  sockpp::tcp_socket sock_;
  std::atomic<bool> running_ = false;

  // Buffer for reading data from the client
  std::shared_ptr<char> buf_;

  // States of the protocol interpreter
  // 0: Not logged in
  // 1: Logged in
  // 2: Handshake passive mode or active mode
  // 3: with Passive mode | Active mode already established:
  //      | Establish data connection based on the mode
  //      | File transfer mode
  //      | File transfer done
  //    loop until the user quits

  // Current working directory
  std::filesystem::path current_working_directory_;

  // Bool variables to check the state of the server
  bool is_username_valid_;
  bool is_logged_in_;

  std::string username_;
  std::string password_;

  // Default to passive mode true (client may be behind a NAT)
  bool is_passive_mode_;
  // Client listening port in active mode
  uint16_t client_data_port_;

  // Check username and password
  void do_user(std::string username);
  void do_pass(std::string password);

  // Set port mode or passive mode
  void do_port(std::string port);
  void do_pasv();

  // Send the file to the client
  void do_retr(std::string filename);
  // Store file to the server, read it from the client socket
  // Then send the response to the client
  void do_stor(std::string filename);
  // List files in the current working directory and send it to the client
  void do_list();
  // Change current working directory, send response to the client
  void do_cwd(std::string directory);
  // Change to parent directory, send response to the client
  void do_cdup(std::string directory);
  // Send the current working directory name to the client
  void do_pwd();
  // Make directory and send request to the client
  void do_mkd(std::string directory);
  // Remove directory and send response to the client
  void do_rmd(std::string directory);
  // Delete file, send response to the client
  void do_dele(std::string filename);
  // Rename from, send response to the client
  void do_rnfr(std::string oldname);
  // Rename to, send response to the client
  void do_rnto(std::string newname);

  // send_file() and recv_file() are used to send and receive files over a
  // socket.
  // These functions will establish a data connection with the client
  // based on the mode (active or passive)
  void send_file(std::string filename);
  void receive_file(std::string filename);

  // Implementation of file() and receive_file() in active mode and
  // passive mode
  void send_file_active(std::string filename);
  void send_file_passive(std::string filename);

  void receive_file_active(std::string filename);
  void receive_file_passive(std::string filename);
};

} // namespace ftp
