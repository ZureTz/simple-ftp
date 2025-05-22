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
  while (running_ && (std::cout << ftp_default_prompt) &&
         std::getline(std::cin, input)) {
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
      do_cdup();
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
void ftp::protocol_interpreter_client::do_list() {
  // Send LIST command to the server
  const std::string list_command = "LIST";
  ftp::send_message(connector_, list_command);

  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, return
  if (response.find("200") == std::string::npos) {
    // Show user the response
    std::cout << response << std::endl;
    return;
  }
  // Otherwise, print the list of files
  std::clog << "[Proto] " << "Listing files in the current directory"
            << std::endl;
  std::cout << response << std::endl;
}

// Change working directory, wait for response
void ftp::protocol_interpreter_client::do_cwd(std::string directory) {
  // Send CWD command to the server
  const std::string command = "CWD " + directory;
  ftp::send_message(connector_, command);
  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, return
  if (response.find("200") == std::string::npos) {
    // Show user the response
    std::cout << response << std::endl;
    return;
  }
  // Otherwise, print the response
  std::clog << "[Proto] " << "Changed working directory to: " << directory
            << std::endl;
  std::cout << response << std::endl;
}

// Change to parent directory, wait for response
void ftp::protocol_interpreter_client::do_cdup() {
  // Use the CWD command to change to parent directory
  do_cwd("..");
}
// Print working directory, wait for response
void ftp::protocol_interpreter_client::do_pwd() {
  // Send PWD command to the server
  const std::string command = "PWD";
  ftp::send_message(connector_, command);
  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, return
  if (response.find("200") == std::string::npos) {
    // Show user the response
    std::cout << response << std::endl;
    return;
  }
  // Otherwise, print the response
  std::clog << "[Proto] " << "Current working directory: " << response
            << std::endl;
  std::cout << response << std::endl;
}

// Make directory, wait for response
void ftp::protocol_interpreter_client::do_mkd(std::string directory) {
  // Send MKD command to the server
  const std::string command = "MKD " + directory;
  ftp::send_message(connector_, command);
  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, return
  if (response.find("200") == std::string::npos) {
    // Show user the response
    std::cout << response << std::endl;
    return;
  }
  // Otherwise, print the response
  std::clog << "[Proto] " << "Created directory: " << directory << std::endl;
  std::cout << response << std::endl;
}

// Remove directory, wait for response
void ftp::protocol_interpreter_client::do_rmd(std::string directory) {
  // Send RMD command to the server
  const std::string command = "RMD " + directory;
  ftp::send_message(connector_, command);
  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, return
  if (response.find("200") == std::string::npos) {
    // Show user the response
    std::cout << response << std::endl;
    return;
  }
  // Otherwise, print the response
  std::clog << "[Proto] " << "Removed directory: " << directory << std::endl;
  std::cout << response << std::endl;
}

// Delete file, wait for response
void ftp::protocol_interpreter_client::do_dele(std::string filename) {
  // Send DELE command to the server
  const std::string command = "DELE " + filename;
  ftp::send_message(connector_, command);
  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, return
  if (response.find("200") == std::string::npos) {
    // Show user the response
    std::cout << response << std::endl;
    return;
  }
  // Otherwise, print the response
  std::clog << "[Proto] " << "Deleted file: " << filename << std::endl;
  std::cout << response << std::endl;
}

// Rename from, wait for response
void ftp::protocol_interpreter_client::do_rnfr(std::string oldname) {
  // Send RNFR command to the server
  const std::string command = "RNFR " + oldname;
  ftp::send_message(connector_, command);
  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, return
  if (response.find("200") == std::string::npos) {
    // Show user the response
    std::cout << response << std::endl;
    return;
  }
  // Otherwise, print the response
  std::clog << "[Proto] " << "Renamed from: " << oldname << std::endl;
  std::cout << response << std::endl;
}

// Rename to, wait for response
void ftp::protocol_interpreter_client::do_rnto(std::string newname) {
  // Send RNTO command to the server
  const std::string command = "RNTO " + newname;
  ftp::send_message(connector_, command);
  // Wait for response from the server
  const auto response = ftp::receive_message(connector_, buf_, buffer_size);
  // If response is not 200, return
  if (response.find("200") == std::string::npos) {
    // Show user the response
    std::cout << response << std::endl;
    return;
  }
  // Otherwise, print the response
  std::clog << "[Proto] " << "Renamed to: " << newname << std::endl;
  std::cout << response << std::endl;
}

// Help command, runs locally without server
void ftp::protocol_interpreter_client::do_help() {
  // Print the help message
  std::cout << "\n===== FTP Client Help =====\n";
  std::cout << "Available commands:\n\n";

  // Authentication commands
  std::cout << "USER <username>  - Specify user for authentication\n";
  std::cout << "PASS <password>  - Specify password for authentication\n";

  // Connection mode commands
  std::cout << "PORT [<port>]    - Use active mode with optional port number\n";
  std::cout << "PASV             - Use passive mode (default)\n";

  // File transfer commands
  std::cout << "RETR <filename>  - Download a file from server\n";
  std::cout << "STOR <filename>  - Upload a file to server\n";
  std::cout << "LIST             - List files in current directory\n";

  // Directory navigation commands
  std::cout << "CWD <directory>  - Change working directory\n";
  std::cout << "CDUP             - Change to parent directory\n";
  std::cout << "PWD              - Print working directory\n";

  // File management commands
  std::cout << "MKD <directory>  - Create a directory\n";
  std::cout << "RMD <directory>  - Remove a directory\n";
  std::cout << "DELE <filename>  - Delete a file\n";
  std::cout << "RNFR <oldname>   - Rename from (specify old filename)\n";
  std::cout << "RNTO <newname>   - Rename to (specify new filename)\n";

  // Other commands
  std::cout << "QUIT             - Exit the FTP client\n";
  std::cout << "HELP             - Show this help message\n";

  std::cout << "\n=========================\n";
}