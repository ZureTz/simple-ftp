#include "proto_interpreter.h"
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

  std::string input;
  while (std::getline(std::cin, input) && running_) {
    // Check if the input contains only whitespace
    if (input.find_first_not_of(" \t\n") == std::string::npos) {
      continue; // Skip empty input
    }

    // Parse the command (feed the command to the ftp::parse_command function)
    auto [operation, argument] = ftp::parse_command(input);

    // Do the do_... functions based on the operation

    // Authentication and quit
    if (operation == ftp::USER) {
      do_user(argument);  
    }
    if (operation == ftp::PASS) {
      do_pass(argument);
    }
    if (operation == ftp::QUIT) {
      stop();
      break;
    }
    
    // Specify active or passive mode (default to passive mode)
    if (operation == ftp::PORT) {
      do_port(argument);
    }
    if (operation == ftp::PASV) {
      do_pasv();
    }

    // File transfer
    if (operation == ftp::RETR) {
      do_retr(argument);
    }
    if (operation == ftp::STOR) {
      do_stor(argument);
    }
    if (operation == ftp::LIST) {
      do_list();
    }
    if (operation == ftp::CWD) {
      do_cwd(argument);
    }
    if (operation == ftp::CDUP) {
      do_cdup(argument);
    }
    if (operation == ftp::PWD) {
      do_pwd();
    }
    if (operation == ftp::MKD) {
      do_mkd(argument);
    }
    if (operation == ftp::RMD) {
      do_rmd(argument);
    }
    if (operation == ftp::DELE) {
      do_dele(argument);
    }
    if (operation == ftp::RNFR) {
      do_rnfr(argument);
    }
    if (operation == ftp::RNTO) {
      do_rnto(argument);
    }

    // Help command
    if (operation == ftp::HELP) {
      do_help();
    }
  }
}

// Stop the protocol interpreter
void ftp::protocol_interpreter_client::stop() {
  // Set running to false
  running_ = false;
}

// Protocol interpreter server implementation
// ...
