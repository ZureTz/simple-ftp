#pragma once
// Header file for file I/O operations using socket

#include <string>

namespace ftp {
// send_file() and recv_file() are used to send and receive files over a socket.
void send_file(int sockfd, std::string filename);
void receive_file(int sockfd, std::string filename);
} // namespace ftp
