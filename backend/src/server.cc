#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>

#include <argparse/argparse.hpp>
#include <sockpp/tcp_acceptor.h>

constexpr int BufferSize = 1024;

void run_echo(sockpp::tcp_socket sock) {
  std::shared_ptr<char> buf(new char[BufferSize],
                            std::default_delete<char[]>());
  ssize_t n;

  while ((n = sock.read(buf.get(), BufferSize)) > 0) {
    // Log the received data
    std::clog << "Received " << n << " bytes from "
              << sock.peer_address().to_string() << ": ";
    for (ssize_t i = 0; i < n; ++i) {
      std::clog << buf.get()[i];
    }
    std::clog << std::endl;
    // Echo the data back to the client
    sock.write(buf.get(), n);
  }

  std::clog << "Connection closed from " << sock.peer_address() << std::endl;
}

int main(int argc, char const *argv[]) {
  // Init argparse
  argparse::ArgumentParser program("simple-ftp-server");
  program.add_argument("-p", "--port")
      .help("Port to listen on")
      .default_value(21)
      .scan<'i', int>();

  // Receive arguments
  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  const int16_t port = program.get<int>("--port");
  std::clog << "Listening on port " << port << std::endl;

  // Init sockpp
  sockpp::initialize();

  // Init tcp acceptor using sockpp
  sockpp::tcp_acceptor acceptor(port);
  if (!acceptor) {
    std::cerr << "Error: " << acceptor.last_error_str() << std::endl;
    return 1;
  }

  // Accept a new client connection
  while (true) {
    sockpp::tcp_socket sock = acceptor.accept();
    if (!sock) {
      std::cerr << "Error: " << sock.last_error_str() << std::endl;
      return 1;
    }

    std::clog << "Accepted connection from " << sock.peer_address().to_string()
              << std::endl;
    // Create a thread and transfer the new stream to it.
    std::thread thr(run_echo, std::move(sock));
    thr.detach();
  }

  return 0;
}