#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include <sockpp/tcp_connector.h>

#include "ftp_client.h"
#include "utils/ftp.h"

// Constructor
ftp::client::client(const std::string &server_host,
                    int16_t server_command_port) {
  // Set server host and port
  server_host_ = server_host;
  server_command_port_ = server_command_port;

  // Set connected to false
  connected_ = false;
}

// Connect to the server
void ftp::client::connect() {
  // Connect to the server
  if (!connector_.connect(
          sockpp::inet_address(server_host_, server_command_port_))) {
    std::cerr << "[Client] " << "Error: " << connector_.last_error_str()
              << std::endl;
    return;
  }

  // Set connected to true
  connected_ = true;

  std::clog << "[Client] " << "Connected to "
            << connector_.peer_address().to_string() << std::endl;
  std::clog << "[Client] " << "Source port: " << connector_.address().port()
            << std::endl;

  // Run the protocol interpreter
  protocol_interpreter_ = new protocol_interpreter_client(&connector_);
  protocol_interpreter_->run();
  // After stop, disconnect from the server
  disconnect();
}

// Disconnect from the server
void ftp::client::disconnect() {
  std::clog << "[Client] " << "Disconnecting from "
            << connector_.peer_address().to_string() << "..." << std::endl;

  // Delete the protocol interpreter
  if (protocol_interpreter_) {
    if (protocol_interpreter_->is_running()) {
      protocol_interpreter_->stop();
    }
    delete protocol_interpreter_;
    protocol_interpreter_ = nullptr;
  }

  // Set connected to false
  connected_ = false;

  // Disconnect from the server
  connector_.close();
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
    std::shared_ptr<char> buffer(new char[buffer_size],
                                 std::default_delete<char[]>());
    ssize_t n = connector_.read(buffer.get(), buffer_size);
    if (n > 0) {
      std::cout.write(buffer.get(), n);
      std::cout << std::endl;
    } else {
      std::cerr << "Error: " << connector_.last_error_str() << std::endl;
      break;
    }
  }
}
