#pragma once

#include <string>
#include <utility>
#include <vector>

namespace ftp {
constexpr int buffer_size = 1024 * 1024; // 1MB

// Use Green Bold text for the prompt
// And arrow using
const std::string ftp_default_prompt = "\033[32;1mftp>\033[34m$\033[0m \033[0m";

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

// Split a string into tokens based on whitespace
std::vector<std::string> split(const std::string &str,
                               const std::vector<std::string> &tokens,
                               char delimiter);

// Parse the command and return the operation
std::pair<operation, std::string> parse_command(std::string command);
} // namespace ftp