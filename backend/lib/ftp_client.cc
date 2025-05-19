#include "ftp_client.h"
#include "utils/ftp.h"

#include <memory>
#include <sockpp/tcp_connector.h>

// Constructor
ftp::client::client(const std::string &server_host,
                    int16_t server_command_port) {

  // Initialize sockpp
  sockpp::initialize();

  // Set server host and port
  server_host_ = server_host;
  server_command_port_ = server_command_port;

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
  if (!connector_.connect(
          sockpp::inet_address(server_host_, server_command_port_))) {
    std::cerr << "Error: " << connector_.last_error_str() << std::endl;
    return;
  }

  // Set connected to true
  connected_ = true;

  std::clog << "Connected to " << connector_.peer_address().to_string()
            << std::endl;
  std::clog << "Source port: " << connector_.address().port() << std::endl;

  // Run the client from the command line
  // run_echo_from_stdin();

  // Run the protocol interpreter
  protocol_interpreter();
}

// Disconnect from the server
void ftp::client::disconnect() {
  // Set connected to false
  connected_ = false;

  std::clog << "Disconnecting from " << connector_.peer_address().to_string()
            << "..." << std::endl;

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
    char buffer[1024];
    ssize_t n = connector_.read(buffer, sizeof(buffer));
    if (n > 0) {
      std::cout.write(buffer, n);
      std::cout << std::endl;
    } else {
      std::cerr << "Error: " << connector_.last_error_str() << std::endl;
      break;
    }
  }
}

// Protocol interpreter
void ftp::client::protocol_interpreter() {
  std::clog << "Protocol interpreter started." << std::endl;
  // Prompt the user for input
  std::cout << "ftp> ";

  std::string input;
  while (std::getline(std::cin, input) && connected_) {
    // Flush the output buffer
    std::cout.flush();
    // Prompt the user for input
    std::cout << "ftp> ";
    // Check if the input contains only whitespace
    if (input.find_first_not_of(" \t\n") == std::string::npos) {
      continue; // Skip empty input
    }

    // Parse the command
    ftp::operation op = ftp::parse_command(input);

    // Check if the command is valid and handle it
    if (op == ftp::operation::USER) {
      std::cout << "User command received." << std::endl;
    } else if (op == ftp::operation::PASS) {
      std::cout << "Password command received." << std::endl;
    } else if (op == ftp::operation::QUIT) {
      std::cout << "Quit command received." << std::endl;
      break; // Exit the loop
    } else if (op == ftp::operation::PORT) {
      std::cout << "Port command received." << std::endl;
    } else if (op == ftp::operation::PASV) {
      std::cout << "Passive mode command received." << std::endl;
    } else if (op == ftp::operation::RETR) {
      do_retr();
    } else if (op == ftp::operation::STOR) {
      do_stor();
    } else if (op == ftp::operation::LIST) {
      do_list();
    } else if (op == ftp::operation::CWD) {
      do_cwd();
    } else if (op == ftp::operation::CDUP) {
      do_cdup();
    } else if (op == ftp::operation::PWD) {
      do_pwd();
    } else if (op == ftp::operation::MKD) {
      do_mkd();
    } else if (op == ftp::operation::RMD) {
      do_rmd();
    } else if (op == ftp::operation::DELE) {
      do_dele();
    } else if (op == ftp::operation::RNFR) {
      do_rnfr();
    } else if (op == ftp::operation::RNTO) {
      do_rnto();
    } else if (op == ftp::operation::HELP) {
      do_help();
    } else {
      std::cerr << "Unknown command: " << input << std::endl;
      continue; // Skip unknown commands
    }
  }
}