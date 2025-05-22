#include "proto/proto_interpreter.h"
#include "utils/ftp.h"
#include "utils/io.h"
#include <string>

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

  // Debug only
  is_logged_in_ = true;

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

// Receive file from the client
void ftp::protocol_interpreter_server::do_stor(std::string filename) {
  // Check if user is already logged in
  if (!is_logged_in_) {
    std::clog << "[Proto] " << "Not logged in" << std::endl;
    const std::string response = "530 Not logged in\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Tell the client that the server is ready to receive the file
  std::string response_one = "200 OK to open data connection\r\n";
  ftp::send_message(&sock_, response_one);
  std::clog << "[Proto] " << "200 OK to open data connection\r\n" << std::endl;

  // Start receiving the file
  std::clog << "[Proto] " << "Receiving file: " << filename << std::endl;
  receive_file(filename);

  // After receiving the file, wait for response from the client
  std::string acknowledge = ftp::receive_message(&sock_, buf_, buffer_size);
  if (acknowledge.find("DONE") == std::string::npos) {
    std::clog << "[Proto] " << "Error: " << acknowledge << std::endl;
    return;
  }
  std::clog << "[Proto] " << "File transfer done" << std::endl;
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

  // Check if the current working directory is valid
  if (!std::filesystem::exists(current_working_directory_)) {
    std::clog << "[Proto] " << "Current working directory does not exist"
              << std::endl;
    const std::string response =
        "550 Current working directory not exist or permission denied.\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // List files in the current working directory
  std::string response = "200 Directory listing:\r\n";
  // Array of file name for further alphabetical sorting
  std::vector<std::string> file_list;
  for (const auto &entry :
       std::filesystem::directory_iterator(current_working_directory_)) {
    // Check if the entry is a file or directory
    if (entry.is_regular_file()) {
      // Add the file name to the list
      file_list.push_back(entry.path().filename().string());
      continue;
    }

    if (entry.is_directory()) {
      // Add the directory name to the list
      file_list.push_back(entry.path().filename().string() + "/");
      continue;
    }
  }
  // Sort the file list (With alphabetical order, and directories first)
  auto str_comp = [](const std::string &a, const std::string &b) {
    // Check for empty strings
    if (a.empty() && b.empty()) {
      return false;
    }
    if (a.empty()) {
      return false;
    }
    if (b.empty()) {
      return true;
    }

    // Sort directories first, then files
    bool a_is_dir = a.back() == '/';
    bool b_is_dir = b.back() == '/';
    if (a_is_dir != b_is_dir) {
      return a_is_dir;
    }

    // Finally sort alphabetically
    return a < b;
  };
  std::sort(file_list.begin(), file_list.end(), str_comp);
  // Add the file names to the response string, note that directories are in
  // blue color
  for (const auto &file : file_list) {
    if (file.back() == '/') {
      // Remove the trailing slash
      std::string file_no_slash = file.substr(0, file.size() - 1);
      response += "\033[34m" + file_no_slash + "\033[0m\r\n";
    } else {
      response += file + "\r\n";
    }
  }

  // Send the response to the client
  ftp::send_message(&sock_, response);
  std::clog << "[Proto] " << "File list sent to client" << std::endl;
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
