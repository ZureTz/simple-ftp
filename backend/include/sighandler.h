#pragma once

#include "ftp_server.h"
#include "ftp_client.h"

#ifdef FTP_SERVER

// Server side
extern ftp::server *ftp_server;
void init_sigint_handler_server();
void sigint_handler_server(int s);

#endif

#ifdef FTP_CLIENT

// Client side
extern ftp::client *ftp_client;
void init_sigint_handler_client();
void sigint_handler_client(int s);

#endif