#ifndef MYTFTPCLIENT_CLIENT_H
#define MYTFTPCLIENT_CLIENT_H

#include <netdb.h>

#define TFTP_PORT "69"

typedef struct TFTPClient {
	int sock;
	struct addrinfo *serv_addr;
	char *mode;
	struct sockaddr_storage tid_addr;
} *client_t;

void client_free(client_t *client);
client_t client_init(char *server);

#endif//MYTFTPCLIENT_CLIENT_H
