#include <cstdlib>
#include <string>
#include <sys/types.h>

#include "proto/proto_interpreter.h"
#include "utils/ftp.h"
#include "utils/io.h"

// Protocol interpreter server implementation
ftp::protocol_interpreter_server::protocol_interpreter_server(
    sockpp::tcp_socket sock) {
  // Set the socket
  sock_ = std::move(sock);
  // Set running to false
  running_ = false;

  // Initialize the buffer
  buf_ = std::shared_ptr<char>(new char[buffer_size],
                               std::default_delete<char[]>());

  // Set the current working directory to the home directory
  current_working_directory_.assign(getenv("HOME"));
  // Log the current working directory
  std::clog << "[Proto] " << "Current working directory: "
            << current_working_directory_.string() << std::endl;
  // Log the relative path
  std::clog << "[Proto] " << "Relative path: "
            << current_working_directory_.relative_path().string() << std::endl;

  // Set is_logged_in to false
  is_username_valid = false;
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
  while (running_) {
    // Read the command from the client
    std::string input = ftp::receive(&sock_, buf_, buffer_size);

    // Parse the command (feed the command to the ftp::parse_command function)
    auto [operation, argument] = ftp::parse_command(input);
    // Log the command
    std::clog << "[Proto] " << "Parsed command: " << operation << " "
              << argument << std::endl;

    // Do the do_... functions based on the operation
    // Authentication and quit
    if (operation == ftp::USER) {
      do_user(argument);
      continue;
    }
    if (operation == ftp::PASS) {
      do_pass(argument);
      continue;
    }

    // Specify active or passive mode (default to passive mode)
    if (operation == ftp::PORT) {
      do_port(argument);
      continue;
    }
    if (operation == ftp::PASV) {
      do_pasv();
      continue;
    }

    // File transfer
    if (operation == ftp::RETR) {
      do_retr(argument);
      continue;
    }
    if (operation == ftp::STOR) {
      do_stor(argument);
      continue;
    }
    if (operation == ftp::LIST) {
      do_list();
      continue;
    }
    if (operation == ftp::CWD) {
      do_cwd(argument);
      continue;
    }
    if (operation == ftp::CDUP) {
      do_cdup(argument);
      continue;
    }
    if (operation == ftp::PWD) {
      do_pwd();
      continue;
    }
    if (operation == ftp::MKD) {
      do_mkd(argument);
      continue;
    }
    if (operation == ftp::RMD) {
      do_rmd(argument);
      continue;
    }
    if (operation == ftp::DELE) {
      do_dele(argument);
      continue;
    }
    if (operation == ftp::RNFR) {
      do_rnfr(argument);
      continue;
    }
    if (operation == ftp::RNTO) {
      do_rnto(argument);
      continue;
    }

    // Quit command
    if (operation == ftp::QUIT) {
      std::clog << "[Proto] " << "Quitting..." << std::endl;
      running_ = false;
      continue;
    }
  }

  // Disconnect from the client
  std::clog << "[Proto] " << "Disconnecting from "
            << sock_.peer_address().to_string() << "..." << std::endl;
  // Stop the protocol interpreter
  stop();
}
// Stop the protocol interpreter
void ftp::protocol_interpreter_server::stop() {
  // Close the socket
  std::clog << "[Proto] " << "Protocol interpreter server for client "
            << sock_.peer_address().to_string() << " stopped" << std::endl;
  sock_.close();
  // End the thread
}

// Is protocol interpreter running?
bool ftp::protocol_interpreter_server::is_running() const { return running_; }

// Check username and password
void ftp::protocol_interpreter_server::do_user(std::string username) {
  // If already logged in, send response
  if (is_logged_in) {
    std::clog << "[Proto] " << "Already logged in" << std::endl;
    const std::string response = "230 User logged in, proceed.\r\n";
    ftp::send(&sock_, response);
    return;
  }

  // Check if the username is already provided
  if (is_username_valid) {
    std::clog << "[Proto] " << "Username already provided" << std::endl;
    const std::string response =
        "331 User name already provided, need password.\r\n";
    ftp::send(&sock_, response);
    return;
  }

  // Check if the username is correct
  if (username != username_) {
    std::clog << "[Proto] " << "Invalid username" << std::endl;
    const std::string response = "530 Not logged in. Invalid username\r\n";
    ftp::send(&sock_, response);
    return;
  }

  // Username is valid
  is_username_valid = true;
  std::clog << "[Proto] " << "Valid username" << std::endl;
  const std::string response = "331 User name okay, need password.\r\n";
  ftp::send(&sock_, response);
}

void ftp::protocol_interpreter_server::do_pass(std::string password) {
  // Check if user is already logged in
  if (is_logged_in) {
    std::clog << "[Proto] " << "Already logged in" << std::endl;
    const std::string response = "230 User logged in, proceed.\r\n";
    ftp::send(&sock_, response);
    return;
  }

  // Check if the username is valid
  if (!is_username_valid) {
    std::clog << "[Proto] " << "Invalid username" << std::endl;
    const std::string response = "530 Not logged in. Invalid username\r\n";
    ftp::send(&sock_, response);
    return;
  }

  // Check if the password is correct
  if (password != password_) {
    std::clog << "[Proto] " << "Invalid password" << std::endl;
    const std::string response = "530 Not logged in. Invalid password\r\n";
    ftp::send(&sock_, response);
    return;
  }

  // Password is valid
  is_logged_in = true;
  std::clog << "[Proto] " << "Valid password" << std::endl;
  const std::string response = "230 User logged in, proceed.\r\n";

  // Send welcome message current working directory
  // Set color green
  const std::string welcome = "\033[32m"
                              "Welcome to the FTP server! "
                              "\033[0m"
                              "\r\n \"" +
                              current_working_directory_.string() +
                              "\" is the current "
                              "directory.\r\n";
  ftp::send(&sock_, response + welcome);
}

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
