#include "utils/io.h"

// Send (using connector)
void ftp::send(sockpp::tcp_connector *connector, const std::string &data) {
  if (!connector) {
    std::cerr << "[IO] " << "Error: connector is null" << std::endl;
    return;
  }

  ssize_t n = connector->write(data);
  if (n <= 0) {
    std::cerr << "[IO] " << "Error: " << connector->last_error_str()
              << std::endl;
    return;
  }

  std::clog << "[IO] " << "Sent data: " << data << std::endl;
}

// Send (using socket)
void ftp::send(sockpp::tcp_socket *socket, const std::string &data) {
  if (!socket) {
    std::cerr << "[IO] " << "Error: socket is null" << std::endl;
    return;
  }

  ssize_t n = socket->write(data);
  if (n <= 0) {
    std::cerr << "[IO] " << "Error: " << socket->last_error_str() << std::endl;
    return;
  }

  std::clog << "[IO] " << "Sent data: " << data << std::endl;
}

// Receive (using connector)
std::string ftp::receive(sockpp::tcp_connector *connector,
                         std::shared_ptr<char> buffer, size_t buffer_size) {
  if (!connector) {
    std::cerr << "[IO] " << "Error: connector is null" << std::endl;
    return "";
  }

  ssize_t response_size = connector->read(buffer.get(), buffer_size);
  if (response_size <= 0) {
    std::cerr << "[IO] " << "Error: " << connector->last_error_str() << std::endl;
    return "";
  }

  std::string response(buffer.get(), response_size);
  std::clog << "[IO] " << "Received data: " << response << std::endl;
  return response;
}

// Receive (using socket)
std::string ftp::receive(sockpp::tcp_socket *socket,
                         std::shared_ptr<char> buffer, size_t buffer_size) {
  if (!socket) {
    std::cerr << "[IO] " << "Error: socket is null" << std::endl;
    return "";
  }

  ssize_t response_size = socket->read(buffer.get(), buffer_size);
  if (response_size <= 0) {
    std::cerr << "[IO] " << "Error: " << socket->last_error_str() << std::endl;
    return "";
  }

  std::string response(buffer.get(), response_size);
  std::clog << "[IO] " << "Received data: " << response << std::endl;
  return response;
}