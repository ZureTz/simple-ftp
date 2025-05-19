#pragma once

#include "ftp_server.h"

extern struct sigaction sig_int_handler;
extern ftp::server *ftp_server;

void init_sigint_handler();
void sigint_handler(int s);