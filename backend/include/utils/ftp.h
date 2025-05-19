#pragma once

#include <string>

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
  RNFR,     // Rename from
  RNTO,     // Rename to
  HELP,     // Help (Print all commands and their description)
};

// Parse the command and return the operation
enum operation parse_command(const std::string &command);
} // namespace ftp