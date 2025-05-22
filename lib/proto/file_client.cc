#include <fcntl.h>
#include <indicators/cursor_control.hpp>
#include <indicators/progress_bar.hpp>
#include <sys/sendfile.h>
#include <sys/stat.h>

#include "proto/proto_interpreter.h"
#include "utils/ftp.h"
#include "utils/io.h"

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