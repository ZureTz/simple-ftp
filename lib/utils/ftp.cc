#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "utils/ftp.h"

// Trim the leading and trailing whitespace from a string
std::string ftp::trim(const std::string &str) {
  auto string_copy = str;
  string_copy = std::regex_replace(string_copy, std::regex("^ +"), "");
  string_copy = std::regex_replace(string_copy, std::regex(" +$"), "");
  return string_copy;
}

// Split a string into tokens based on whitespace
std::vector<std::string> ftp::split(const std::string &str,
                                    const std::vector<std::string> &tokens,
                                    char delimiter) {

  // Clear the passed tokens vector
  std::vector<std::string> return_tokens;

  std::string token;
  std::istringstream token_stream(str);
  // Using istringstream to split the string by whitespace
  while (std::getline(token_stream, token, delimiter)) {
    // Skip empty tokens (consecutive delimiters will produce empty tokens)
    if (token.empty()) {
      continue;
    }
    return_tokens.push_back(token);
  }

  return return_tokens;
}

// Parse the command and return the operation based on the command
std::pair<ftp::operation, std::string> ftp::parse_command(std::string command) {
  // Remove leading and trailing whitespace
  command = ftp::trim(command);

  // Separate the command and the argument by space
  std::vector<std::string> tokens;
  tokens = split(command, tokens, ' ');

  if (tokens.empty()) {
    return {ftp::NOOP, ""}; // No operation
  }

  // Convert the first command to lowercase
  std::transform(tokens[0].begin(), tokens[0].end(), tokens[0].begin(),
                 [](unsigned char c) { return std::tolower(c); });
  // Check the command and return the corresponding operation
  // Log the command
  std::clog << "[Parser] Parsed command: ";
  for (const auto &token : tokens) {
    std::clog << token << " ";
  }
  std::clog << std::endl;

  // user <username>
  if (tokens[0] == "user" && tokens.size() == 2) {
    return {ftp::USER, tokens[1]};
  }
  // pass <password>
  if (tokens[0] == "pass" && tokens.size() == 2) {
    return {ftp::PASS, tokens[1]};
  }
  // quit
  if (tokens[0] == "quit" && tokens.size() == 1) {
    return {ftp::QUIT, ""};
  }
  // port <port> (has default)
  if (tokens[0] == "port" && (tokens.size() == 2 || tokens.size() == 1)) {
    return {ftp::PORT, tokens.size() == 2 ? tokens[1] : ""};
  }
  // pasv
  if (tokens[0] == "pasv" && tokens.size() == 1) {
    return {ftp::PASV, ""};
  }
  // retr <filename>
  if ((tokens[0] == "retr" || tokens[0] == "get") && tokens.size() == 2) {
    return {ftp::RETR, tokens[1]};
  }
  // stor <filename>
  if ((tokens[0] == "stor" || tokens[0] == "put") && tokens.size() == 2) {
    return {ftp::STOR, tokens[1]};
  }
  // list
  if ((tokens[0] == "list" || tokens[0] == "ls" || tokens[0] == "dir") &&
      tokens.size() == 1) {
    return {ftp::LIST, ""};
  }
  // cwd <directory>
  if ((tokens[0] == "cwd" || tokens[0] == "cd") && tokens.size() == 2) {
    return {ftp::CWD, tokens[1]};
  }
  // cdup
  if ((tokens[0] == "cdup" || tokens[0] == "cd..") && tokens.size() == 1) {
    return {ftp::CDUP, ""};
  }
  // pwd
  if (tokens[0] == "pwd" && tokens.size() == 1) {
    return {ftp::PWD, ""};
  }
  // mkd <directory>
  if ((tokens[0] == "mkd" || tokens[0] == "mkdir") && tokens.size() == 2) {
    return {ftp::MKD, tokens[1]};
  }
  // rmd <directory>
  if ((tokens[0] == "rmd" || tokens[0] == "rmdir") && tokens.size() == 2) {
    return {ftp::RMD, tokens[1]};
  }
  // dele <filename>
  if ((tokens[0] == "dele" || tokens[0] == "rm") && tokens.size() == 2) {
    return {ftp::DELE, tokens[1]};
  }
  // rnfr <oldname>
  if (tokens[0] == "rnfr" && tokens.size() == 2) {
    return {ftp::RNFR, tokens[1]};
  }
  // rnto <newname>
  if (tokens[0] == "rnto" && tokens.size() == 2) {
    return {ftp::RNTO, tokens[1]};
  }
  // help
  if ((tokens[0] == "help" || tokens[0] == "?") && tokens.size() == 1) {
    return {ftp::HELP, ""};
  }
  // noop (invalid command)
  return {ftp::NOOP, ""};
}