#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
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
  is_username_valid_ = false;
  is_logged_in_ = false;

  // Set the username and password from environment variables
  username_ = getenv("FTP_USERNAME") ? getenv("FTP_USERNAME") : "anonymous";
  password_ = getenv("FTP_PASSWORD") ? getenv("FTP_PASSWORD") : "anonymous";
  // Debug current username and password
  std::clog << "[Proto] " << "Username: " << username_ << std::endl;
  std::clog << "[Proto] " << "Password: " << password_ << std::endl;

  // Set the default to passive mode
  is_passive_mode_ = true;

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
    std::string input = ftp::receive_message(&sock_, buf_, buffer_size);

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
  if (is_logged_in_) {
    std::clog << "[Proto] " << "Already logged in" << std::endl;
    const std::string response = "230 User logged in, proceed.\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the username is already provided
  if (is_username_valid_) {
    std::clog << "[Proto] " << "Username already provided" << std::endl;
    const std::string response =
        "331 User name already provided, need password.\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the username is correct
  if (username != username_) {
    std::clog << "[Proto] " << "Invalid username" << std::endl;
    const std::string response = "530 Not logged in. Invalid username\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Username is valid
  is_username_valid_ = true;
  std::clog << "[Proto] " << "Valid username" << std::endl;
  const std::string response = "331 User name okay, need password.\r\n";
  ftp::send_message(&sock_, response);
}

void ftp::protocol_interpreter_server::do_pass(std::string password) {
  // Check if user is already logged in
  if (is_logged_in_) {
    std::clog << "[Proto] " << "Already logged in" << std::endl;
    const std::string response = "230 User logged in, proceed.\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the username is valid
  if (!is_username_valid_) {
    std::clog << "[Proto] " << "Invalid username" << std::endl;
    const std::string response = "530 Not logged in. Invalid username\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the password is correct
  if (password != password_) {
    std::clog << "[Proto] " << "Invalid password" << std::endl;
    const std::string response = "530 Not logged in. Invalid password\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Password is valid
  is_logged_in_ = true;
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
  ftp::send_message(&sock_, response + welcome);
}

// Set port mode or passive mode
void ftp::protocol_interpreter_server::do_port(std::string port) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // If the port is empty, send response
  if (port.empty()) {
    std::clog << "[Proto] " << "Port is setting to default port" << std::endl;
    // Use default port (client port  + 1)
    const int default_port_num = sock_.peer_address().port() + 1;

    // Check if the port number is valid
    if (default_port_num < 1023 || default_port_num > 65535) {
      std::clog << "[Proto] " << "Invalid port number" << std::endl;
      const std::string response = "500 Invalid port number\r\n";
      ftp::send_message(&sock_, response);
      return;
    }

    // Set passive mode false
    is_passive_mode_ = false;

    // Set client_port_ to default port
    client_data_port_ = uint16_t(default_port_num);

    // Tell the client that the port is set
    std::clog << "[Proto] " << "Port set to " << client_data_port_ << std::endl;
    const std::string response =
        "200 Port set to " + std::to_string(client_data_port_) + "\r\n";
    ftp::send_message(&sock_, response);

    return;
  }

  // Remove leading and trailing whitespace
  port = ftp::trim(port);

  // Convert the port string to an integer
  const int port_num = std::stoi(port);

  // Check if the port number is valid
  if (port_num < 1024 || port_num > 65535) {
    std::clog << "[Proto] " << "Invalid port number" << std::endl;
    const std::string response = "500 Invalid port number\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Set passive mode false
  is_passive_mode_ = false;

  // Set client_port_ to provided port
  client_data_port_ = uint16_t(port_num);

  // Tell the client that the port is set
  std::clog << "[Proto] " << "Port set to " << client_data_port_ << std::endl;
  const std::string response =
      "200 Port set to " + std::to_string(client_data_port_) + "\r\n";
  ftp::send_message(&sock_, response);
}

void ftp::protocol_interpreter_server::do_pasv() {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Set passive mode true
  is_passive_mode_ = true;

  // Log result
  std::clog << "[Proto] " << "Passive mode set" << std::endl;

  // Tell the client that the port is set
  const std::string response = "200 Passive mode set to true.\r\n";
  ftp::send_message(&sock_, response);
}

// Send the file to the client
void ftp::protocol_interpreter_server::do_retr(std::string filename) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the file exists
  // Get path by filename
  std::filesystem::path file_path = current_working_directory_ / filename;
  if (!std::filesystem::exists(file_path)) {
    std::clog << "[Proto] " << "File \"" << file_path << "\" does not exist"
              << std::endl;
    const std::string response = "550 File not found\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
  // File exists, tell the client that the file is ready to be sent
  std::string response_one = "200 File status okay; about to open data "
                             "connection\r\n";
  ftp::send_message(&sock_, response_one);
  std::clog << "[Proto] "
            << "File status okay; about to open data "
               "connection"
            << std::endl;

  // Start sending the file
  std::clog << "[Proto] " << "Sending file: " << filename << std::endl;
  send_file(filename);

  // After sending the file, wait for response from the client
  std::string acknowledge = ftp::receive_message(&sock_, buf_, buffer_size);
  if (acknowledge.find("DONE") == std::string::npos) {
    std::clog << "[Proto] " << "Error: " << acknowledge << std::endl;
    return;
  }
  std::clog << "[Proto] " << "File transfer done" << std::endl;
}

// Store file to the server, read it from the client socket
// Then send the response to the client
void ftp::protocol_interpreter_server::do_stor(std::string filename) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
}

// List files in the current working directory and send it to the client
void ftp::protocol_interpreter_server::do_list() {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
}

// Change current working directory, send response to the client
void ftp::protocol_interpreter_server::do_cwd(std::string directory) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
}

// Change to parent directory, send response to the client
void ftp::protocol_interpreter_server::do_cdup(std::string directory) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
}

// Send the current working directory name to the client
void ftp::protocol_interpreter_server::do_pwd() {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
}

// Make directory and send request to the client
void ftp::protocol_interpreter_server::do_mkd(std::string directory) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
}

// Remove directory and send response to the client
void ftp::protocol_interpreter_server::do_rmd(std::string directory) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
}

// Delete file, send response to the client
void ftp::protocol_interpreter_server::do_dele(std::string filename) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
}

// Rename from, send response to the client
void ftp::protocol_interpreter_server::do_rnfr(std::string oldname) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
}

// Rename to, send response to the client
void ftp::protocol_interpreter_server::do_rnto(std::string newname) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }
}

// send_file() and recv_file() are used to send and receive files over a
// socket.
// These functions will establish a data connection with the client
// based on the mode (active or passive)
void ftp::protocol_interpreter_server::send_file(std::string filename) {
  // Check if using the active mode or passive mode
  if (is_passive_mode_) {
    send_file_passive(filename);
    return;
  }

  // Active mode
  send_file_active(filename);
}

void ftp::protocol_interpreter_server::receive_file(std::string filename) {
  // Check if using the active mode or passive mode
  if (is_passive_mode_) {
    receive_file_passive(filename);
    return;
  }

  // Active mode
  receive_file_active(filename);
}

// Implementation of file() and receive_file() in active mode and
// passive mode
void ftp::protocol_interpreter_server::send_file_active(std::string filename) {
  std::clog << "[Proto][File] " << "Sending file in active mode not implemented"
            << std::endl;
}

void ftp::protocol_interpreter_server::send_file_passive(std::string filename) {
  // Create a new buffer for the file
  std::shared_ptr<char> file_buf = std::shared_ptr<char>(
      new char[buffer_size], std::default_delete<char[]>());
  // Create a connection to the server using a new sockpp::tcp_acceptor
  sockpp::tcp_acceptor data_acceptor(sock_.address().port() + 1);
  if (!data_acceptor) {
    std::cerr << "Error: " << data_acceptor.last_error_str() << std::endl;
    return;
  }

  // Next: send the file to the client using established data connection
  const auto file_path =
      current_working_directory_ / filename; // Get the file path
  // Log the file path
  std::clog << "[Proto][File] " << "File path: " << file_path.string()
            << std::endl;
  int send_file_fd = open(file_path.c_str(), O_RDONLY);
  if (send_file_fd == -1) {
    std::cerr << "Error: " << strerror(errno) << std::endl;
    data_acceptor.close();
    return;
  }

  // Get the file status
  struct stat file_stat;
  if (fstat(send_file_fd, &file_stat) == -1) {
    std::cerr << "Error: " << strerror(errno) << std::endl;
    close(send_file_fd);
    data_acceptor.close();
    return;
  }

  // Log the file size
  std::clog << "[Proto][File] " << "File size: " << file_stat.st_size
            << std::endl;

  // Accept a new connection from the client
  sockpp::tcp_socket data_sock = data_acceptor.accept();
  if (!data_sock) {
    std::cerr << "Error: " << data_acceptor.last_error_str() << std::endl;
    close(send_file_fd);
    data_acceptor.close();
    return;
  }
  std::clog << "[Proto][File] "
            << "Accepted data connection from " << data_sock.peer_address()
            << std::endl;
  // Send file size to the client
  std::string file_size_str = std::to_string(file_stat.st_size) + "\r\n";
  // Using sock_ instead of data_sock to send the file size
  // to prevent collision with the data connection
  ftp::send_message(&sock_, file_size_str);

  // Send the file to the client
  off_t offset = 0;
  size_t remaining_size = file_stat.st_size;
  while (remaining_size > 0) {
    const auto sent_bytes =
        sendfile(data_sock.handle(), send_file_fd, &offset, remaining_size);
    if (sent_bytes < 0) {
      std::cerr << "Error: " << strerror(errno) << std::endl;
      break;
    }
    // fprintf(stdout, "1. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
    std::clog << "[Proto][File] "
              << "1. Server sent " << sent_bytes
              << " bytes from file's data, offset is now: " << offset
              << " and remaining data: " << remaining_size << std::endl;
    remaining_size -= sent_bytes;
    // fprintf(stdout, "2. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
    std::clog << "[Proto][File] "
              << "2. Server sent " << sent_bytes
              << " bytes from file's data, offset is now: " << offset
              << " and remaining data: " << remaining_size << std::endl;
  }

  // Close the file descriptor
  close(send_file_fd);
  // Close the data socket
  data_sock.close();
  // Close the data acceptor
  data_acceptor.close();
}

void ftp::protocol_interpreter_server::receive_file_active(
    std::string filename) {
  std::clog << "[Proto][File] "
            << "Receiving file in active mode not implemented" << std::endl;
}

void ftp::protocol_interpreter_server::receive_file_passive(
    std::string filename) {
  std::clog << "[Proto][File] "
            << "Receiving file in passive mode not implemented" << std::endl;
}
