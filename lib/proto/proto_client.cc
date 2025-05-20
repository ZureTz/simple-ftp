#include <cstdlib>
#include <iostream>
#include <string>

#include "proto/proto_interpreter.h"
#include "utils/ftp.h"

// Protocol interpreter client implementation
// Constructor
ftp::protocol_interpreter_client::protocol_interpreter_client(
    sockpp::tcp_connector *const connector) {
  connector_ = connector;
}

// Run the protocol interpreter
void ftp::protocol_interpreter_client::run() {
  // Set running to true
  running_ = true;

  // Print the welcome message
  std::clog << "[Proto] " << "Welcome to the FTP client!" << std::endl;

  std::string input;
  while ((std::cout << "ftp> ") && std::getline(std::cin, input) && running_) {
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
    ssize_t n = connector_->write(input.c_str(), input.size());
    if (n <= 0) {
      std::cerr << "Error: " << connector_->last_error_str() << std::endl;
      break;
    }

    // Authentication and quit
    // if (operation == ftp::USER) {
    //   do_user(argument);
    //   continue;
    // }
    // if (operation == ftp::PASS) {
    //   do_pass(argument);
    //   continue;
    // }
    // if (operation == ftp::QUIT) {
    //   stop();
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

    // // Help command
    // if (operation == ftp::HELP) {
    //   do_help();
    //   continue;
    // }

    // // NOOP command
    // if (operation == ftp::NOOP) {
    //   std::clog<< "[Proto] " << "No operation" << std::endl;
    //   continue;
    // }
  }
}

// Stop the protocol interpreter
void ftp::protocol_interpreter_client::stop() {
  // Set running to false
  running_ = false;
}

// Send username to the server, wait for response
void ftp::protocol_interpreter_client::do_user(std::string username) {}
// Send password to the server, wait for response
void ftp::protocol_interpreter_client::do_pass(std::string password) {}

// Todo: Specify active or passive mode
// Send PORT command to the server, wait for response
void ftp::protocol_interpreter_client::do_port(std::string port) {}
// Send PASV command to the server, wait for response
void ftp::protocol_interpreter_client::do_pasv() {}

// Todo: implement the FTP commands
// Retrieve file from the server, save it to the local file system
// And wait for response
void ftp::protocol_interpreter_client::do_retr(std::string filename) {}
// Store file to the server, read it from the local file system
// And wait for response
void ftp::protocol_interpreter_client::do_stor(std::string filename) {}
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
void ftp::protocol_interpreter_client::send_file(std::string filename) {}
void ftp::protocol_interpreter_client::receive_file(std::string filename) {}