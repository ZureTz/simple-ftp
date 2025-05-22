// Simple echo client using sockpp

#include <string>

#include <argparse/argparse.hpp>

#include "ftp_client.h"
#include "utils/sighandler.h"

// ftp client pointer for the signal handler
ftp::client *ftp_client = nullptr;

int main(int argc, char const *argv[]) {
  // Init argparse
  argparse::ArgumentParser program("simple-ftp-client");
  program.add_argument("-p", "--port")
      .help("Port to connect to")
      .default_value(21)
      .scan<'i', int>();

  program.add_argument("-h", "--host")
      .help("Host to connect to")
      .default_value("localhost");

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
  const std::string host = program.get<std::string>("--host");
  std::clog << "[Main] " << "Connecting to " << host << ":" << port
            << std::endl;

  // Init client
  ftp::client client(host, port);
  // Init signal handler
  init_sigint_handler_client();

  // Connect to the server
  client.connect();

  return 0;
}