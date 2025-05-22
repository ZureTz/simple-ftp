#include "utils/io.h"

// Send (using connector)
void ftp::send_message(sockpp::tcp_connector *connector,
                       const std::string &data) {
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

  // Only log the first line of the data
  const size_t line_end = data.find('\n');
  std::string first_line = data.substr(0, line_end);
  // Remove trailing \r
  if (first_line.back() == '\r') {
    first_line.pop_back();
  }
  std::clog << "[IO] " << "Sent data: " << first_line
            << (line_end == data.size() - 1 ? "" : "...") << "[" << data.size()
            << " bytes]" << std::endl;
}

// Send (using socket)
void ftp::send_message(sockpp::tcp_socket *socket, const std::string &data) {
  if (!socket) {
    std::cerr << "[IO] " << "Error: socket is null" << std::endl;
    return;
  }

  ssize_t n = socket->write(data);
  if (n <= 0) {
    std::cerr << "[IO] " << "Error: " << socket->last_error_str() << std::endl;
    return;
  }

  // Only log the first line of the data
  const size_t line_end = data.find('\n');
  std::string first_line = data.substr(0, line_end);
  // Remove trailing \r
  if (first_line.back() == '\r') {
    first_line.pop_back();
  }
  std::clog << "[IO] " << "Sent data: " << first_line
            << (line_end == data.size() - 1 ? "" : "...") << "[" << data.size()
            << " bytes]" << std::endl;
}

// Receive (using connector)
std::string ftp::receive_message(sockpp::tcp_connector *connector,
                                 std::shared_ptr<char> buffer,
                                 size_t buffer_size) {
  if (!connector) {
    std::cerr << "[IO] " << "Error: connector is null" << std::endl;
    return "";
  }

  ssize_t response_size = connector->read(buffer.get(), buffer_size);
  if (response_size <= 0) {
    std::cerr << "[IO] " << "Error: " << connector->last_error_str()
              << std::endl;
    return "";
  }

  std::string response(buffer.get(), response_size);
  // Only log the first line of the response
  const size_t line_end = response.find('\n');
  std::string first_line = response.substr(0, line_end);
  // Remove trailing \r
  if (first_line.back() == '\r') {
    first_line.pop_back();
  }
  std::clog << "[IO] " << "Received data: " << first_line
            << (line_end == response.size() - 1 ? "" : "...") << "["
            << response.size() << " bytes]" << std::endl;
  return response;
}

// Receive (using socket)
std::string ftp::receive_message(sockpp::tcp_socket *socket,
                                 std::shared_ptr<char> buffer,
                                 size_t buffer_size) {
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
  // Only log the first line of the response
  const size_t line_end = response.find('\n');
  std::string first_line = response.substr(0, line_end);
  // Remove trailing \r
  if (first_line.back() == '\r') {
    first_line.pop_back();
  }
  std::clog << "[IO] " << "Received data: " << first_line
            << (line_end == response.size() - 1 ? "" : "...") << "["
            << response.size() << " bytes]" << std::endl;

  return response;
}