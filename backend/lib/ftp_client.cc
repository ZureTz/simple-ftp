#include "ftp_client.h"

#include <sockpp/tcp_connector.h>

// Constructor
ftp::client::client(const std::string &host, int16_t port) {
  // Initialize sockpp
  sockpp::initialize();

  // Host and port
  host_ = host;
  port_ = port;

  // Set connected to false
  connected_ = false;
}

// Destructor
ftp::client::~client() {
  // Disconnect if connected
  if (connected_) {
    disconnect();
  }
}

// Connect to the server
void ftp::client::connect() {
  // Connect to the server
  if (!connector_.connect(sockpp::inet_address(host_, port_))) {
    std::cerr << "Error: " << connector_.last_error_str() << std::endl;
    return;
  }

  // Set connected to true
  connected_ = true;

  std::clog << "Connected to " << connector_.peer_address().to_string()
            << std::endl;
}

// Disconnect from the server
void ftp::client::disconnect() {
  // Disconnect from the server
  connector_.close();

  // Set connected to false
  connected_ = false;

  std::clog << "Disconnected from " << connector_.peer_address().to_string()
            << std::endl;
}

// Run the client from the command line
void ftp::client::run_echo_from_stdin() {
  std::string input;
  while (std::getline(std::cin, input) && input != "exit" && connected_) {
    // Check if the input contains only whitespace
    if (input.find_first_not_of(" \t\n") == std::string::npos) {
      continue; // Skip empty input
    }

    // Send the input to the server
    connector_.write(input.c_str(), input.size());
    // Read the response from the server
    char buf[1024];
    ssize_t n = connector_.read(buf, sizeof(buf));
    if (n > 0) {
      std::cout.write(buf, n);
      std::cout << std::endl;
    } else {
      std::cerr << "Error: " << connector_.last_error_str() << std::endl;
      break;
    }
  }
}