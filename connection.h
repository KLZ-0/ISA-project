#ifndef MYTFTPCLIENT_CONNECTION_H
#define MYTFTPCLIENT_CONNECTION_H

#include "client.h"

#define BUFF_SIZE 1024
#define ACK_MSG_SIZE 4

int conn_send_init(client_t client, char *filename, opcode_t opcode);
int conn_recv(client_t client);

#endif//MYTFTPCLIENT_CONNECTION_H
