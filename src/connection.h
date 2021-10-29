#ifndef MYTFTPCLIENT_CONNECTION_H
#define MYTFTPCLIENT_CONNECTION_H

#include "client.h"

#define ACK_SIZE 4
#define RESEND_COUNT_MAX 2 ///< How many times should we try to resend DATA packets
#define DEFAULT_TIMEOUT 3

int conn_init(client_t client);
int conn_recv(client_t client);
int conn_send(client_t client);

#endif //MYTFTPCLIENT_CONNECTION_H
