#ifndef MYTFTPCLIENT_CONNECTION_H
#define MYTFTPCLIENT_CONNECTION_H

#include "client.h"

#define BUFF_SIZE 1024

int conn_send_rrq(client_t client, char *filename);
int conn_recv(client_t client);

#endif//MYTFTPCLIENT_CONNECTION_H
