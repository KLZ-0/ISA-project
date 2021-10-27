#ifndef MYTFTPCLIENT_CONNECTION_H
#define MYTFTPCLIENT_CONNECTION_H

#include "client.h"

#define ACK_MSG_SIZE 4

// TODO: make variable
#define BLOCK_SIZE 512

int conn_send_init(client_t client, opcode_t opcode);
int conn_recv(client_t client);

#endif //MYTFTPCLIENT_CONNECTION_H
