#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "utils/sighandler.h"

struct sigaction sig_int_handler;

#ifdef FTP_SERVER

void init_sigint_handler_server() {
  sig_int_handler.sa_handler = sigint_handler_server;
  sigemptyset(&sig_int_handler.sa_mask);
  sig_int_handler.sa_flags = 0;

  sigaction(SIGINT, &sig_int_handler, NULL);
}

void sigint_handler_server(int s) {
  std::clog << "\r[Signal] Caught signal " << s << std::endl;

  // Stop the server
  ftp_server->stop();

  exit(1);
}

#endif

#ifdef FTP_CLIENT

void init_sigint_handler_client() {
  sig_int_handler.sa_handler = sigint_handler_client;
  sigemptyset(&sig_int_handler.sa_mask);
  sig_int_handler.sa_flags = 0;

  sigaction(SIGINT, &sig_int_handler, NULL);
}

void sigint_handler_client(int s) {
  std::clog << "\r[Signal] Caught signal " << s << std::endl;

  ftp_client->disconnect();

  exit(1);
}

#endif