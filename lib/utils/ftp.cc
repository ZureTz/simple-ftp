#include <algorithm>
#include <cstddef>
#include <iostream>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "utils/ftp.h"

std::pair<ftp::operation, std::string> ftp::parse_command(std::string command) {
  auto split = [](const std::string &txt, std::vector<std::string> &strs,
                  char ch) -> size_t {
    size_t pos = txt.find(ch);
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while (pos != std::string::npos) {
      strs.push_back(txt.substr(initialPos, pos - initialPos));
      initialPos = pos + 1;

      pos = txt.find(ch, initialPos);
    }

    // Add the last one
    strs.push_back(
        txt.substr(initialPos, std::min(pos, txt.size()) - initialPos + 1));

    return strs.size();
  };

  // Remove leading and trailing whitespace
  command = std::regex_replace(command, std::regex("^ +"), "");
  command = std::regex_replace(command, std::regex(" +$"), "");

  // Separate the command and the argument by space
  std::vector<std::string> tokens;
  split(command, tokens, ' ');

  if (tokens.empty()) {
    return {ftp::NOOP, ""}; // No operation
  }

  // Convert the first command to lowercase
  std::transform(tokens[0].begin(), tokens[0].end(), tokens[0].begin(),
                 [](unsigned char c) { return std::tolower(c); });
  // Check the command and return the corresponding operation
  // Log the command
  std::clog << "[Parser] Parsed command: " ;
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
  if (tokens[0] == "rnfr"&& tokens.size() == 2) {
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