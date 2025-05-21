#pragma once

#include <memory>
#include <string>

#include <sockpp/tcp_connector.h>
#include <sockpp/tcp_socket.h>

namespace ftp {

// Send (using connector)
void send_message(sockpp::tcp_connector *connector, const std::string &data);

// Send (using socket)
void send_message(sockpp::tcp_socket *socket, const std::string &data);

// Receive (using connector)
std::string receive_message(sockpp::tcp_connector *connector,
                            std::shared_ptr<char> buffer, size_t buffer_size);

// Receive (using socket)
std::string receive_message(sockpp::tcp_socket *socket,
                            std::shared_ptr<char> buffer, size_t buffer_size);
} // namespace ftp