#include "sighandler.h"

#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct sigaction sig_int_handler;

void init_sigint_handler() {
  sig_int_handler.sa_handler = sigint_handler;
  sigemptyset(&sig_int_handler.sa_mask);
  sig_int_handler.sa_flags = 0;

  sigaction(SIGINT, &sig_int_handler, NULL);
}

void sigint_handler(int s) {
  std::clog << "\rCaught signal " << s << std::endl;

  ftp_server->stop();

  exit(1);
}
