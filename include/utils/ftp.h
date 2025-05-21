#pragma once

#include <string>
#include <utility>

constexpr int buffer_size = 1024;

namespace ftp {
// FTP operations
enum operation {
  USER = 0, // User name
  PASS,     // Password
  QUIT,     // Quit
  PORT,     // Port mode
  PASV,     // Passive mode
  RETR,     // Retrieve file
  STOR,     // Store file
  LIST,     // List files (ls)
  CWD,      // Change working directory (cd <dir>)
  CDUP,     // Change to parent directory (cd ..)
  PWD,      // Print working directory (pwd)
  MKD,      // Make directory (mkdir <dir>)
  RMD,      // Remove directory (rmdir <dir>)
  DELE,     // Delete
  RNFR,     // Rename from (rnfr <old>)
  RNTO,     // Rename to (rnto <new>)
  HELP,     // Help (Print all commands and their description)
  NOOP,     // No operation
};

// Trim the leading and trailing whitespace from a string
std::string trim(const std::string &str);

// Parse the command and return the operation
std::pair<operation, std::string> parse_command(std::string command);
} // namespace ftp