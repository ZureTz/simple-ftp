// Simple echo client using sockpp

#include <argparse/argparse.hpp>
#include <sockpp/tcp_connector.h>
#include <string>

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

  const int16_t port = program.get<int>("--port");
  const std::string host = program.get<std::string>("--host");
  std::clog << "Connecting to " << host << ":" << port << std::endl;

  // Init sockpp
  sockpp::initialize();

  // Init tcp connector using sockpp
  sockpp::tcp_connector connector;

  if (!connector.connect(sockpp::inet_address(host, port))) {
    std::cerr << "Error: " << connector.last_error_str() << std::endl;
    return 1;
  }

  std::clog << "Connected to " << connector.peer_address().to_string()
            << std::endl;

  std::string input;
  while (std::getline(std::cin, input) && input != "exit") {
    // Check if the input contains only whitespace
    if (input.find_first_not_of(" \t\n") == std::string::npos) {
      continue; // Skip empty input
    }

    // Send the input to the server
    connector.write(input.c_str(), input.size());
    // Read the response from the server
    char buf[1024];
    ssize_t n = connector.read(buf, sizeof(buf));
    if (n > 0) {
      std::cout.write(buf, n);
      std::cout << std::endl;
    } else {
      std::cerr << "Error: " << connector.last_error_str() << std::endl;
      break;
    }
  }

  // Close the connection
  connector.close();

  return 0;
}