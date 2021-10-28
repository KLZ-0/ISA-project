#ifndef MYTFTPCLIENT_CONNECTION_H
#define MYTFTPCLIENT_CONNECTION_H

#include "client.h"

#define ACK_MSG_SIZE 4

int conn_init(client_t client);
int conn_recv(client_t client);
int conn_send(client_t client);

#endif //MYTFTPCLIENT_CONNECTION_H
