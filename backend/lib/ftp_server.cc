#include <memory>
#include <thread>
#include <unistd.h>

#include "ftp_server.h"
#include "utils/ftp.h"

// Constructor
ftp::server::server(int16_t command_port) {
  // Initialize sockpp
  sockpp::initialize();

  // Port number
  command_port_ = command_port;

  // Set running to false
  running_ = false;
}

// Destructor
ftp::server::~server() {
  // Stop the server
  stop();
}

// Start the server
void ftp::server::start() {
  // Create a TCP acceptor
  const bool err = acceptor_.open(sockpp::inet_address(command_port_));
  if (!err) {
    std::cerr << "Error: " << acceptor_.last_error_str() << std::endl;
    return;
  }

  // Start the server
  running_ = true;
  std::clog << "Server started on command port " << command_port_ << std::endl;

  // Accept a new client connection
  while (running_) {
    sockpp::tcp_socket sock = acceptor_.accept();
    if (!sock) {
      std::cerr << "Error: " << acceptor_.last_error_str() << std::endl;
      return;
    }

    std::clog << "Accepted connection from " << sock.peer_address().to_string()
              << std::endl;

    // After command port connection, we need use protocol interpreter
    std::thread thr(&ftp::server::protocol_interpreter, this, std::move(sock));
    thr.detach(); // Detach the thread to allow it to run independently
  }
}

// Stop the server
void ftp::server::stop() {
  // Stop the server
  running_ = false;

  // Close the acceptor
  acceptor_.close();
  std::clog << "Server stopped." << std::endl;
}

void ftp::server::run_echo(sockpp::tcp_socket sock) {
  std::shared_ptr<char> buf(new char[buffer_size],
                            std::default_delete<char[]>());
  ssize_t n;
  while ((n = sock.read(buf.get(), buffer_size)) > 0) {
    // Log the received data
    std::clog << "Received " << n << " bytes from "
              << sock.peer_address().to_string() << ": ";
    for (ssize_t i = 0; i < n; ++i) {
      std::clog << buf.get()[i];
    }
    std::clog << std::endl;
    // Echo the data back to the client
    sock.write(buf.get(), n);
  }

  std::clog << "Connection closed from " << sock.peer_address() << std::endl;
}

// Protocol interpreter
void ftp::server::protocol_interpreter(sockpp::tcp_socket sock) {
  std::shared_ptr<char> buf(new char[buffer_size],
                            std::default_delete<char[]>());
  ssize_t n;

  while ((n = sock.read(buf.get(), buffer_size)) > 0) {
    // Log the received data
    std::clog << "Received " << n << " bytes from "
              << sock.peer_address().to_string() << ": ";
    for (ssize_t i = 0; i < n; ++i) {
      std::clog << buf.get()[i];
    }
    std::clog << std::endl;
    // Echo the data back to the client
    sock.write(buf.get(), n);
  }

  std::clog << "Connection closed from " << sock.peer_address() << std::endl;
}