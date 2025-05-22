#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <indicators/cursor_control.hpp>
#include <indicators/progress_bar.hpp>
#include <sockpp/tcp_acceptor.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#include "proto/proto_interpreter.h"
#include "utils/ftp.h"
#include "utils/io.h"

// Protocol interpreter client implementation
// Constructor
ftp::protocol_interpreter_client::protocol_interpreter_client(
    sockpp::tcp_connector *const connector) {
  // Set the connector
  connector_ = connector;
  // Set running to false
  running_ = false;
  // Initialize the buffer
  buf_ = std::shared_ptr<char>(new char[buffer_size],
                               std::default_delete<char[]>());
  // Set the default to passive mode
  is_passive_mode_ = true;

  // Set the default client data port to current port + 1 (active mode)
  client_data_port_ = uint16_t(connector_->address().port() + 1);
}

// Run the protocol interpreter
void ftp::protocol_interpreter_client::run() {
  // Set running to true
  running_ = true;

  // Print the welcome message
  std::clog << "[Proto] " << "Welcome to the FTP client!" << std::endl;

  std::string input;
  while (running_ && (std::cout << "ftp> ") && std::getline(std::cin, input)) {
    // Check if the input contains only whitespace
    if (input.find_first_not_of(" \t\n") == std::string::npos) {
      continue; // Skip empty input
    }

    // Parse the command (feed the command to the ftp::parse_command function)
    auto [operation, argument] = ftp::parse_command(input);

    // Do the do_... functions based on the operation

    // Debugging: send the command to the server
    std::clog << "[Proto] " << "Sending command: " << operation << " "
              << argument << std::endl;
    // Send the command to the server (debug only)
    // ssize_t n = connector_->write(input.c_str(), input.size());
    // if (n <= 0) {
    //   std::cerr << "Error: " << connector_->last_error_str() << std::endl;
    //   break;
    // }

    // Authentication and quit
    if (operation == ftp::USER) {
      do_user(argument);
      continue;
    }
    if (operation == ftp::PASS) {
      do_pass(argument);
      continue;
    }
    if (operation == ftp::QUIT) {
      std::clog << "[Proto] " << "Quitting..." << std::endl;
      stop();
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

    // Help command
    if (operation == ftp::HELP) {
      do_help();
      continue;
    }

    // NOOP command
    if (operation == ftp::NOOP) {
      std::clog << "[Proto] " << "Invalid command." << std::endl;
      continue;
    }
  }
}

// Stop the protocol interpreter
void ftp::protocol_interpreter_client::stop() {
  // Set running to false
  running_ = false;
  // Send QUIT command to the server
  std::string quit_command = "QUIT";
  ftp::send_message(connector_, quit_command);
}

// Check if the protocol interpreter is running
bool ftp::protocol_interpreter_client::is_running() const { return running_; }

// Send username to the server, wait for response
void ftp::protocol_interpreter_client::do_user(std::string username) {
  const std::string user_command = "USER " + username;
  ftp::send_message(connector_, user_command);

  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  std::cout << response << std::endl;
}

// Send password to the server, wait for response
void ftp::protocol_interpreter_client::do_pass(std::string password) {
  const std::string pass_command = "PASS " + password;
  ftp::send_message(connector_, pass_command);

  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  std::cout << response << std::endl;
}

// Specify active or passive mode
void ftp::protocol_interpreter_client::do_port(std::string port) {
  const std::string port_command = "PORT " + port;
  ftp::send_message(connector_, port_command);

  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If the response is not 200, remain client_port_ and is_passive_mode_
  // unchanged
  if (response.find("200") == std::string::npos) {
    // Log the response
    std::clog << response << std::endl;
    // Show user the response
    std::cout << response << std::endl;
    return;
  }

  // Otherwise, set client_port_ to the port number and set is_passive_mode_ to
  // false
  is_passive_mode_ = false;

  // If the port number is not specified (empty), set it to the default port
  if (port.empty()) {
    std::clog << "[Proto] " << "Port is setting to default port" << std::endl;
    // Use default port (client port  + 1)
    const int default_port_num = connector_->address().port() + 1;

    // Set client_port_ to default port
    client_data_port_ = uint16_t(default_port_num);

    // Print the port number
    std::clog << "[Proto] " << "Port set to " << client_data_port_ << std::endl;
    // Print the response to the user
    std::cout << response << std::endl;
    return;
  }

  // Use the port number from argument
  port = ftp::trim(port);
  // Convert the port string to an integer
  const int port_num = std::stoi(port);
  // Since the port number is already checked in the server, we can safely
  // set client_port_ to the port number
  client_data_port_ = uint16_t(port_num);

  // Print the port number
  std::clog << "[Proto] " << "Port set to " << client_data_port_ << std::endl;
  // Print the response to the user
  std::cout << response << std::endl;
}

// Send PASV command to the server, wait for response
void ftp::protocol_interpreter_client::do_pasv() {
  // Send PASV command to the server
  const std::string pasv_command = "PASV";
  ftp::send_message(connector_, pasv_command);

  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, remain is_passive_mode_ unchanged
  if (response.find("200") == std::string::npos) {
    // Log the response
    std::clog << response << std::endl;
    // Show user the response
    std::cout << response << std::endl;
    return;
  }

  // Otherwise, set is_passive_mode_ to true
  is_passive_mode_ = true;

  // Log the response
  std::clog << "[Proto] " << "Passive mode set" << std::endl;

  // Show user the response
  std::cout << response << std::endl;
}

// Retrieve file from the server, save it to the local file system
// And wait for response
void ftp::protocol_interpreter_client::do_retr(std::string filename) {
  // Send RETR command to the server
  const std::string retr_command = "RETR " + filename;
  ftp::send_message(connector_, retr_command);
  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, return
  if (response.find("200") == std::string::npos) {
    // Show user the response
    std::cout << response << std::endl;
    return;
  }

  // Server is ready to send the file, prepare to receive the file
  std::clog << "[Proto] " << "Receiving file: " << filename << std::endl;
  receive_file(filename);

  // After sending the file, tell the server that sending is done
  const std::string done_command = "DONE";
  ftp::send_message(connector_, done_command);
  // Log that the file is done
  std::clog << "[Proto] " << "File transfer done" << std::endl;
}
// Store file to the server, read it from the local file system
// And wait for response
void ftp::protocol_interpreter_client::do_stor(std::string filename) {
  // Check if the file exists in the local file system
  if (!std::filesystem::exists(filename)) {
    std::clog << "[Proto] " << "File \"" << filename << "\" does not exist"
              << std::endl;
    return;
  }

  // Send STOR command to the server
  const std::string retr_command = "STOR " + filename;
  ftp::send_message(connector_, retr_command);

  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, return
  if (response.find("200") == std::string::npos) {
    // Show user the response
    std::cout << response << std::endl;
    return;
  }

  // Server is ready to send the file, prepare to send the file
  std::clog << "[Proto] " << "Sending file: " << filename << std::endl;
  send_file(filename);

  // After sending the file, tell the server that sending is done
  const std::string done_command = "DONE";
  ftp::send_message(connector_, done_command);
  // Log that the file is done
  std::clog << "[Proto] " << "File transfer done" << std::endl;
}
// List files in the current directory, wait for response
void ftp::protocol_interpreter_client::do_list() {}
// Change working directory, wait for response
void ftp::protocol_interpreter_client::do_cwd(std::string directory) {}
// Change to parent directory, wait for response
void ftp::protocol_interpreter_client::do_cdup(std::string directory) {}
// Print working directory, wait for response
void ftp::protocol_interpreter_client::do_pwd() {}
// Make directory, wait for response
void ftp::protocol_interpreter_client::do_mkd(std::string directory) {}
// Remove directory, wait for response
void ftp::protocol_interpreter_client::do_rmd(std::string directory) {}
// Delete file, wait for response
void ftp::protocol_interpreter_client::do_dele(std::string filename) {}
// Rename from, wait for response
void ftp::protocol_interpreter_client::do_rnfr(std::string oldname) {}
// Rename to, wait for response
void ftp::protocol_interpreter_client::do_rnto(std::string newname) {}

// Help command, runs locally without server
void ftp::protocol_interpreter_client::do_help() {}

// send_file() and recv_file() are used to send and receive files over a
// socket.
// These functions will establish a data connection with the client
// based on the mode (active or passive)
void ftp::protocol_interpreter_client::send_file(std::string filename) {
  // Check if using passive mode or active mode
  if (is_passive_mode_) {
    send_file_passive(filename);
    return;
  }

  // Active mode
  send_file_active(filename);
}

void ftp::protocol_interpreter_client::receive_file(std::string filename) {
  // Check if using passive mode or active mode
  if (is_passive_mode_) {
    receive_file_passive(filename);
    return;
  }

  // Active mode
  receive_file_active(filename);
}

// Implementation of file() and receive_file() in active mode and
// passive mode

void ftp::protocol_interpreter_client::send_file_active(std::string filename) {
  // Create a connection to the server using a new sockpp::tcp_acceptor
  sockpp::tcp_acceptor data_acceptor(client_data_port_);
  if (!data_acceptor) {
    std::cerr << "Error: " << data_acceptor.last_error_str() << std::endl;
    return;
  }

  // Log the file name
  std::clog << "[Proto][File] " << "File name: " << filename << std::endl;
  int send_file_fd = open(filename.c_str(), O_RDONLY);
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

  // Accept a new connection from the server
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
  // Send file size to the server
  std::string file_size_str = std::to_string(file_stat.st_size) + "\r\n";
  // Using sock_ instead of data_sock to send the file size
  // to prevent collision with the data connection
  ftp::send_message(connector_, file_size_str);

  // Send the file to the server
  off_t offset = 0;
  size_t remaining_size = file_stat.st_size;
  while (remaining_size > 0) {
    const auto sent_bytes =
        sendfile(data_sock.handle(), send_file_fd, &offset, remaining_size);
    if (sent_bytes < 0) {
      std::cerr << "Error: " << strerror(errno) << std::endl;
      break;
    }
    // fprintf(stdout, "1. Client sent %d bytes from file's data, offset is now
    // : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
    std::clog << "[Proto][File] "
              << "1. Client sent " << sent_bytes
              << " bytes from file's data, offset is now: " << offset
              << " and remaining data: " << remaining_size << std::endl;
    remaining_size -= sent_bytes;
    // fprintf(stdout, "2. Client sent %d bytes from file's data, offset is now
    // : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
    std::clog << "[Proto][File] "
              << "2. Client sent " << sent_bytes
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

void ftp::protocol_interpreter_client::send_file_passive(std::string filename) {
  // Log the file name
  std::clog << "[Proto][File] " << "File name: " << filename << std::endl;
  int send_file_fd = open(filename.c_str(), O_RDONLY);
  if (send_file_fd == -1) {
    std::cerr << "Error: " << strerror(errno) << std::endl;
    return;
  }

  // Get the file status
  struct stat file_stat;
  if (fstat(send_file_fd, &file_stat) == -1) {
    std::cerr << "Error: " << strerror(errno) << std::endl;
    close(send_file_fd);
    return;
  }

  // Log the file size
  std::clog << "[Proto][File] " << "File size: " << file_stat.st_size
            << std::endl;

  // Sleep for 500ms to wait for the server to listen on the port
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  // Create a connection to the server using a new sockpp::tcp_connector
  sockpp::tcp_connector data_connector(
      sockpp::inet_address(connector_->peer_address().address(),
                           connector_->peer_address().port() + 1));
  if (!data_connector) {
    std::cerr << "Error: " << data_connector.last_error_str() << std::endl;
    close(send_file_fd);
    return;
  }

  // Send the file to the server using established data connection
  std::clog << "[Proto][File] "
            << "Established data connection to "
            << data_connector.peer_address() << std::endl;

  // Send file size to the server
  std::string file_size_str = std::to_string(file_stat.st_size) + "\r\n";
  // Using sock_ instead of data_sock to send the file size
  // to prevent collision with the data connection
  ftp::send_message(connector_, file_size_str);

  // Send the file to the server
  off_t offset = 0;
  size_t remaining_size = file_stat.st_size;
  while (remaining_size > 0) {
    const auto sent_bytes = sendfile(data_connector.handle(), send_file_fd,
                                     &offset, remaining_size);
    if (sent_bytes < 0) {
      std::cerr << "Error: " << strerror(errno) << std::endl;
      break;
    }
    // fprintf(stdout, "1. Client sent %d bytes from file's data, offset is now
    // : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
    std::clog << "[Proto][File] "
              << "1. Client sent " << sent_bytes
              << " bytes from file's data, offset is now: " << offset
              << " and remaining data: " << remaining_size << std::endl;
    remaining_size -= sent_bytes;
    // fprintf(stdout, "2. Client sent %d bytes from file's data, offset is now
    // : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
    std::clog << "[Proto][File] "
              << "2. Client sent " << sent_bytes
              << " bytes from file's data, offset is now: " << offset
              << " and remaining data: " << remaining_size << std::endl;
  }

  // Close the file descriptor
  close(send_file_fd);
  // Close the data socket
  data_connector.close();
}

// Receive file from the server using active mode
void ftp::protocol_interpreter_client::receive_file_active(
    std::string filename) {
  // Create a new buffer to receive the file
  std::shared_ptr<char> file_buf(new char[buffer_size],
                                 std::default_delete<char[]>());
  // Create a connection to the server using a new sockpp::tcp_acceptor
  sockpp::tcp_acceptor data_acceptor(
      sockpp::inet_address(connector_->address().address(), client_data_port_));
  // Accept a new connection from the server
  sockpp::tcp_socket data_sock = data_acceptor.accept();

  // Receive the file size from the server
  const auto file_size_str =
      ftp::receive_message(connector_, buf_, buffer_size);
  // Convert the file size string to an integer
  const long file_size = std::stoi(file_size_str);
  std::clog << "[Proto][File] "
            << "File size to receive: " << file_size << std::endl;

  // Create a new file to save the received file
  auto const receive_file_fd = fopen(filename.c_str(), "w");
  if (receive_file_fd == nullptr) {
    std::cerr << "Error: " << strerror(errno) << std::endl;
    data_sock.close();
    data_acceptor.close();
    return;
  }

  // Hide cursor
  indicators::show_console_cursor(false);

  // Prepare the progress bar using indicators
  indicators::ProgressBar bar{
      indicators::option::BarWidth{30},
      indicators::option::ShowElapsedTime{true},
      indicators::option::ShowRemainingTime{true},
      indicators::option::PrefixText{"Downloading "},
      indicators::option::ForegroundColor{indicators::Color::green},
      indicators::option::ShowPercentage{true},
      indicators::option::FontStyles{
          std::vector<indicators::FontStyle>{indicators::FontStyle::bold},
      },
  };

  bool successful = true;
  long remaining_size = file_size;
  while (remaining_size > 0) {
    // Receive the file data from the server
    const ssize_t n = data_sock.read(file_buf.get(), buffer_size);
    if (n <= 0) {
      std::cerr << "Error: " << data_sock.last_error_str() << std::endl;
      successful = false;
      break;
    }

    // Write the received data to the file
    fwrite(file_buf.get(), sizeof(char), n, receive_file_fd);
    remaining_size -= n;

    // If completed, skip the progress bar
    if (bar.is_completed()) { // Set the progress bar value
      continue;
    }
    // Otherwise, set the progress bar to the current value
    // Update the progress bar
    bar.set_progress(100 - (remaining_size * 100) / file_size);
  }

  if (successful) {
    // Completed, set the progress bar to 100%
    bar.set_option(indicators::option::PrefixText{"Download complete "});
    bar.mark_as_completed();
  } else {
    // Error occurred, set the progress bar to error
    bar.set_option(indicators::option::PrefixText{"Download failed "});
    bar.mark_as_completed();
  }

  // Show cursor
  indicators::show_console_cursor(true);

  // Close the file
  fclose(receive_file_fd);
  // Close the data connection
  data_sock.close();
  // Close the data acceptor
  data_acceptor.close();
}

// Receive file from the server using passive mode
void ftp::protocol_interpreter_client::receive_file_passive(
    std::string filename) {
  // Create a new buffer to receive the file
  std::shared_ptr<char> file_buf(new char[buffer_size],
                                 std::default_delete<char[]>());
  // Sleep for 500ms to wait for the server to listen on the port
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  // Create a connection to the server using a new sockpp::tcp_connector
  sockpp::tcp_connector data_connector(
      sockpp::inet_address(connector_->peer_address().address(),
                           connector_->peer_address().port() + 1));
  if (!data_connector) {
    std::cerr << "Error: " << data_connector.last_error_str() << std::endl;
    return;
  }

  // Receive the file size from the server
  const auto file_size_str =
      ftp::receive_message(connector_, buf_, buffer_size);
  // Convert the file size string to an integer
  const long file_size = std::stoi(file_size_str);
  std::clog << "[Proto][File] "
            << "File size to receive: " << file_size << std::endl;

  // Create a new file to save the received file
  auto const receive_file_fd = fopen(filename.c_str(), "w");
  if (receive_file_fd == nullptr) {
    std::cerr << "Error: " << strerror(errno) << std::endl;
    data_connector.close();
    return;
  }

  // Hide cursor
  indicators::show_console_cursor(false);

  // Prepare the progress bar using indicators
  indicators::ProgressBar bar{
      indicators::option::BarWidth{30},
      indicators::option::ShowElapsedTime{true},
      indicators::option::ShowRemainingTime{true},
      indicators::option::PrefixText{"Downloading "},
      indicators::option::ForegroundColor{indicators::Color::green},
      indicators::option::ShowPercentage{true},
      indicators::option::FontStyles{
          std::vector<indicators::FontStyle>{indicators::FontStyle::bold},
      },
  };

  bool successful = true;
  long remaining_size = file_size;
  while (remaining_size > 0) {
    // Receive the file data from the server
    const ssize_t n = data_connector.read(file_buf.get(), buffer_size);
    if (n <= 0) {
      std::cerr << "Error: " << data_connector.last_error_str() << std::endl;
      successful = false;
      break;
    }

    // Write the received data to the file
    fwrite(file_buf.get(), sizeof(char), n, receive_file_fd);
    remaining_size -= n;

    // If completed, skip the progress bar
    if (bar.is_completed()) { // Set the progress bar value
      continue;
    }
    // Otherwise, set the progress bar to the current value
    // Update the progress bar
    bar.set_progress(100 - (remaining_size * 100) / file_size);
  }

  if (successful) {
    // Completed, set the progress bar to 100%
    bar.set_option(indicators::option::PrefixText{"Download complete "});
    bar.mark_as_completed();
  } else {
    // Error occurred, set the progress bar to error
    bar.set_option(indicators::option::PrefixText{"Download failed "});
    bar.mark_as_completed();
  }

  // Show cursor
  indicators::show_console_cursor(true);

  // Close the file
  fclose(receive_file_fd);
  // Close the data connection
  data_connector.close();
}