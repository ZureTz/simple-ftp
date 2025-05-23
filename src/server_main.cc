// Simple echo server using sockpp

#include <iostream>

#include <argparse/argparse.hpp>

#include "ftp_server.h"
#include "utils/sighandler.h"

ftp::server *ftp_server = nullptr;

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

  // Initialize sockpp
  sockpp::initialize();

  const uint16_t port = program.get<int>("--port");
  std::clog << "[Main] " << "Listening on port " << port << std::endl;

  // Init server
  ftp::server server(port);

  // Pass the server to the signal handler
  ftp_server = &server;
  // Init signal handler
  init_sigint_handler_server();

  // Start the server
  server.start();

  return 0;
}