#include "proto/proto_interpreter.h"
#include "utils/ftp.h"

// Protocol interpreter server implementation
ftp::protocol_interpreter_server::protocol_interpreter_server(
    sockpp::tcp_socket sock) {
  // Set the socket
  sock_ = std::move(sock);
  // Set running to false
  running_ = false;

  // Set the current working directory to the home directory
  current_working_directory_.assign("/home/parallels");
  // Log the current working directory
  std::clog << "[Proto] " << "Current working directory: "
            << current_working_directory_.string() << std::endl;
  // Log the relative path
  std::clog << "[Proto] " << "Relative path: "
            << current_working_directory_.relative_path().string() << std::endl;

  // Set is_logged_in to false
  is_logged_in = false;
  // Set the username and password from environment variables
  username_ = getenv("FTP_USERNAME") ? getenv("FTP_USERNAME") : "anonymous";
  password_ = getenv("FTP_PASSWORD") ? getenv("FTP_PASSWORD") : "anonymous";
  // Debug current username and password
  std::clog << "[Proto] " << "Username: " << username_ << std::endl;
  std::clog << "[Proto] " << "Password: " << password_ << std::endl;

  // Set the default to passive mode
  is_passive_mode = true;

  // log
  std::clog << "[Proto] " << "Successfully created protocol interpreter server"
            << std::endl;
}

// Run the protocol interpreter
void ftp::protocol_interpreter_server::run() {
  // Set running to true
  running_ = true;
  // Keep receiving commands from the client
  // Init command buffer
  std::shared_ptr<char> buf(new char[buffer_size],
                            std::default_delete<char[]>());
  while (running_) {
    // Read the command from the client
    ssize_t n = sock_.read(buf.get(), buffer_size);
    if (n <= 0) {
      std::cerr << "[Proto] " << "Error: " << sock_.last_error_str()
                << std::endl;
      break;
    }

    // Log the received data
    std::clog << "[Proto] " << "Received " << n << " bytes from "
              << sock_.peer_address().to_string() << ": ";
    for (ssize_t i = 0; i < n; ++i) {
      std::clog << buf.get()[i];
    }
    std::clog << std::endl;

    // Convert the buffer to a string
    std::string input(buf.get(), n);

    // Parse the command (feed the command to the ftp::parse_command function)
    auto [operation, argument] = ftp::parse_command(input);
    // Log the command
    std::clog << "[Proto] " << "Parsed command: " << operation << " "
              << argument << std::endl;

    // Do the do_... functions based on the operation
    // Authentication and quit
    // if (operation == ftp::USER) {
    //   do_user(argument);
    //   continue;
    // }
    // if (operation == ftp::PASS) {
    //   do_pass(argument);
    //   continue;
    // }

    // // Specify active or passive mode (default to passive mode)
    // if (operation == ftp::PORT) {
    //   do_port(argument);
    //   continue;
    // }
    // if (operation == ftp::PASV) {
    //   do_pasv();
    //   continue;
    // }

    // // File transfer
    // if (operation == ftp::RETR) {
    //   do_retr(argument);
    //   continue;
    // }
    // if (operation == ftp::STOR) {
    //   do_stor(argument);
    //   continue;
    // }
    // if (operation == ftp::LIST) {
    //   do_list();
    //   continue;
    // }
    // if (operation == ftp::CWD) {
    //   do_cwd(argument);
    //   continue;
    // }
    // if (operation == ftp::CDUP) {
    //   do_cdup(argument);
    //   continue;
    // }
    // if (operation == ftp::PWD) {
    //   do_pwd();
    //   continue;
    // }
    // if (operation == ftp::MKD) {
    //   do_mkd(argument);
    //   continue;
    // }
    // if (operation == ftp::RMD) {
    //   do_rmd(argument);
    //   continue;
    // }
    // if (operation == ftp::DELE) {
    //   do_dele(argument);
    //   continue;
    // }
    // if (operation == ftp::RNFR) {
    //   do_rnfr(argument);
    //   continue;
    // }
    // if (operation == ftp::RNTO) {
    //   do_rnto(argument);
    //   continue;
    // }
  }

  // Disconnect from the client
  std::clog << "[Proto] " << "Disconnecting from "
            << sock_.peer_address().to_string() << "..." << std::endl;
  // Stop the protocol interpreter
  stop();
}
// Stop the protocol interpreter
void ftp::protocol_interpreter_server::stop() {
  // Set running to false
  running_ = false;
  // Close the socket
  sock_.close();
  std::clog << "[Proto] " << "Protocol interpreter server stopped."
            << std::endl;
}

// Check username and password
void ftp::protocol_interpreter_server::do_user(std::string username) {}
void ftp::protocol_interpreter_server::do_pass(std::string password) {}

// Set port mode or passive mode
void ftp::protocol_interpreter_server::do_port(std::string port) {}
void ftp::protocol_interpreter_server::do_pasv() {}

// Send the file to the client
void ftp::protocol_interpreter_server::do_retr(std::string filename) {}
// Store file to the server, read it from the client socket
// Then send the response to the client
void ftp::protocol_interpreter_server::do_stor(std::string filename) {}
// List files in the current working directory and send it to the client
void ftp::protocol_interpreter_server::do_list() {}
// Change current working directory, send response to the client
void ftp::protocol_interpreter_server::do_cwd(std::string directory) {}
// Change to parent directory, send response to the client
void ftp::protocol_interpreter_server::do_cdup(std::string directory) {}
// Send the current working directory name to the client
void ftp::protocol_interpreter_server::do_pwd() {}
// Make directory and send request to the client
void ftp::protocol_interpreter_server::do_mkd(std::string directory) {}
// Remove directory and send response to the client
void ftp::protocol_interpreter_server::do_rmd(std::string directory) {}
// Delete file, send response to the client
void ftp::protocol_interpreter_server::do_dele(std::string filename) {}
// Rename from, send response to the client
void ftp::protocol_interpreter_server::do_rnfr(std::string oldname) {}
// Rename to, send response to the client
void ftp::protocol_interpreter_server::do_rnto(std::string newname) {}

// send_file() and recv_file() are used to send and receive files over a
// socket.
// These functions will establish a data connection with the client
// based on the mode (active or passive)
void ftp::protocol_interpreter_server::send_file(std::string filename) {}
void ftp::protocol_interpreter_server::receive_file(std::string filename) {}
