#include "proto/proto_interpreter.h"
#include "utils/ftp.h"
#include "utils/io.h"
#include <iostream>
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
  // is_logged_in_ = true;

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

    // Quit command
    if (operation == ftp::QUIT) {
      std::clog << "[Proto] " << "Quitting..." << std::endl;
      running_ = false;
      continue;
    }

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

    // Check if user is already logged in
    // Otherwise, they cannot do further operations
    if (!is_logged_in_) {
      std::clog << "[Proto] " << "Not logged in" << std::endl;
      const std::string response = "530 Not logged in\r\n";
      ftp::send_message(&sock_, response);
      continue;
    }

    if (operation == ftp::RNTO) {
      do_rnto(argument);
      continue;
    }

    // Check if user is in a "RNFR" -> "RNTO" state
    if (!rename_oldname_path_.empty()) {
      std::clog << "[Proto] " << "Should use RNTO command" << std::endl;
      const std::string response = "503 RNFR command not completed\r\n";
      ftp::send_message(&sock_, response);
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

    // File operations
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
  std::string response = "200 Directory listing:\r\n\n";
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
      response += "    \e[1m\033[34m" + file_no_slash + "\033[0m\e[m\r\n";
      continue;
    }
    response += "    " + file + "\r\n";
  }

  // Send the response to the client
  ftp::send_message(&sock_, response);
  std::clog << "[Proto] " << "File list sent to client" << std::endl;
}

// Change current working directory, send response to the client
void ftp::protocol_interpreter_server::do_cwd(std::string directory) {
  // Check if the directory is "."
  if (directory == ".") {
    std::clog << "[Proto] " << "Current working directory is already "
              << current_working_directory_.string() << std::endl;
    const std::string response = "200 Directory changed to " +
                                 current_working_directory_.string() + "\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the directory is ".."
  if (directory == "..") {
    // Change to parent directory
    current_working_directory_ = current_working_directory_.parent_path();
    std::clog << "[Proto] " << "Changed working directory to "
              << current_working_directory_.string() << std::endl;
    const std::string response = "200 Directory changed to " +
                                 current_working_directory_.string() + "\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the directory is valid
  std::filesystem::path new_directory = current_working_directory_ / directory;
  if (!std::filesystem::exists(new_directory)) {
    std::clog << "[Proto] " << "Directory \"" << new_directory
              << "\" does not exist" << std::endl;
    const std::string response = "550 Directory not found\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the directory is a directory
  if (!std::filesystem::is_directory(new_directory)) {
    std::clog << "[Proto] " << "Path \"" << new_directory
              << "\" is not a directory" << std::endl;
    const std::string response = "550 Path is not a directory\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Change the current working directory
  current_working_directory_ = new_directory;
  std::clog << "[Proto] " << "Changed working directory to "
            << current_working_directory_.string() << std::endl;
  // Send response to the client
  const std::string response = "200 Directory changed to " +
                               current_working_directory_.string() + "\r\n";
  ftp::send_message(&sock_, response);
}

// Change to parent directory, send response to the client
void ftp::protocol_interpreter_server::do_cdup() {
  // Just use cwd ..
  do_cwd("..");
}

// Send the current working directory name to the client
void ftp::protocol_interpreter_server::do_pwd() {
  // Check if the current working directory is valid
  if (!std::filesystem::exists(current_working_directory_)) {
    std::clog << "[Proto] " << "Current working directory does not exist"
              << std::endl;
    const std::string response =
        "550 Current working directory not exist or permission denied.\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Send the current working directory to the client
  std::clog << "[Proto] " << "Current working directory: "
            << current_working_directory_.string() << std::endl;
  std::string response =
      "200 Current working directory: " + current_working_directory_.string() +
      "\r\n";
  ftp::send_message(&sock_, response);
}

// Make directory and send response to the client
void ftp::protocol_interpreter_server::do_mkd(std::string directory) {
  // Check if the directory is "." or ".."
  if (directory == "." || directory == "..") {
    std::clog << "[Proto] " << "Invalid directory name" << std::endl;
    const std::string response = "550 Invalid directory name\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the directory already exists
  std::filesystem::path new_directory = current_working_directory_ / directory;
  if (std::filesystem::exists(new_directory)) {
    std::clog << "[Proto] " << "Directory \"" << new_directory
              << "\" already exists" << std::endl;
    const std::string response = "550 Directory already exists\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Create the directory
  if (!std::filesystem::create_directory(new_directory)) {
    std::clog << "[Proto] " << "Failed to create directory \"" << new_directory
              << "\"" << std::endl;
    const std::string response = "550 Failed to create directory\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Directory created successfully
  std::clog << "[Proto] " << "Directory \"" << new_directory
            << "\" created successfully" << std::endl;
  const std::string response = "200 Directory created successfully\r\n";
  ftp::send_message(&sock_, response);
}

// Remove directory and send response to the client
void ftp::protocol_interpreter_server::do_rmd(std::string directory) {
  // Check if the directory is "." or ".."
  if (directory == "." || directory == "..") {
    std::clog << "[Proto] " << "Invalid directory name" << std::endl;
    const std::string response = "550 Invalid directory name\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the directory exists
  std::filesystem::path new_directory = current_working_directory_ / directory;
  if (!std::filesystem::exists(new_directory)) {
    std::clog << "[Proto] " << "Directory \"" << new_directory
              << "\" does not exist" << std::endl;
    const std::string response = "550 Directory does not exist\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the directory is a directory
  if (!std::filesystem::is_directory(new_directory)) {
    std::clog << "[Proto] " << "Path \"" << new_directory
              << "\" is not a directory" << std::endl;
    const std::string response = "550 Path is not a directory\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the directory is empty
  if (!std::filesystem::is_empty(new_directory)) {
    std::clog << "[Proto] " << "Directory \"" << new_directory
              << "\" is not empty" << std::endl;
    const std::string response = "550 Directory is not empty\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Remove the directory
  if (!std::filesystem::remove(new_directory)) {
    std::clog << "[Proto] " << "Failed to remove directory \"" << new_directory
              << "\"" << std::endl;
    const std::string response = "550 Failed to remove directory\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Directory removed successfully
  std::clog << "[Proto] " << "Directory \"" << new_directory
            << "\" removed successfully" << std::endl;
  const std::string response = "200 Directory removed successfully\r\n";
  ftp::send_message(&sock_, response);
}

// Delete file, send response to the client
void ftp::protocol_interpreter_server::do_dele(std::string filename) {
  // Check if the file exists
  std::filesystem::path file_path = current_working_directory_ / filename;
  if (!std::filesystem::exists(file_path)) {
    std::clog << "[Proto] " << "File \"" << file_path << "\" does not exist"
              << std::endl;
    const std::string response = "550 File not found\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the file is a file
  if (!std::filesystem::is_regular_file(file_path)) {
    std::clog << "[Proto] " << "Path \"" << file_path
              << "\" is not a regular file" << std::endl;
    const std::string response = "550 Path is not a regular file\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Remove the file
  if (!std::filesystem::remove(file_path)) {
    std::clog << "[Proto] " << "Failed to remove file \"" << file_path << "\""
              << std::endl;
    const std::string response = "550 Failed to remove file\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // File removed successfully
  std::clog << "[Proto] " << "File \"" << file_path << "\" removed successfully"
            << std::endl;
  const std::string response = "200 File removed successfully\r\n";
  ftp::send_message(&sock_, response);
}

// Rename from, send response to the client
void ftp::protocol_interpreter_server::do_rnfr(std::string oldname) {
  // Check if oldname is "." or ".."
  if (oldname == "." || oldname == "..") {
    std::clog << "[Proto] " << "Invalid file name" << std::endl;
    const std::string response = "550 Invalid file name\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the file exists
  std::filesystem::path file_path = current_working_directory_ / oldname;
  if (!std::filesystem::exists(file_path)) {
    std::clog << "[Proto] " << "File \"" << file_path << "\" does not exist"
              << std::endl;
    const std::string response = "550 File not found\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Either this is a file or directory is ok
  if (!std::filesystem::is_regular_file(file_path) &&
      !std::filesystem::is_directory(file_path)) {
    std::clog << "[Proto] " << "Path \"" << file_path
              << "\" is not a regular file or directory" << std::endl;
    const std::string response =
        "550 Path is not a regular file or directory\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // File exists, put the file path in the rename_stack_
  rename_oldname_path_ = file_path;

  // Tell the client that the file is ready to be renamed
  std::clog << "[Proto] " << "File status okay; about to rename file"
            << std::endl;
  std::string response_one = "200 File status okay; about to rename file\r\n";
  ftp::send_message(&sock_, response_one);
}

// Rename to, send response to the client
void ftp::protocol_interpreter_server::do_rnto(std::string newname) {
  // Check if rename_oldname_ is empty
  // If is empty, it means that the user has not used RNFR command
  if (rename_oldname_path_.empty()) {
    std::clog << "[Proto] " << "No file to rename" << std::endl;
    const std::string response = "503 No file to rename\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if newname is "." or ".."
  if (newname == "." || newname == "..") {
    std::clog << "[Proto] " << "Invalid file name" << std::endl;
    const std::string response = "550 Invalid file name\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Check if the file exists
  std::filesystem::path new_file_path = current_working_directory_ / newname;
  if (std::filesystem::exists(new_file_path)) {
    std::clog << "[Proto] " << "File \"" << new_file_path << "\" already exists"
              << std::endl;
    const std::string response = "550 File already exists\r\n";
    ftp::send_message(&sock_, response);
    return;
  }

  // Rename the file
  std::filesystem::rename(rename_oldname_path_, new_file_path);

  // File renamed successfully
  std::clog << "[Proto] " << "File \"" << rename_oldname_path_
            << "\" renamed to \"" << new_file_path << "\"" << std::endl;
  const std::string response = "200 File renamed successfully\r\n";
  ftp::send_message(&sock_, response);

  // After renaming, clear the rename_oldname_
  rename_oldname_path_.clear();
}
