#ifndef MYTFTPCLIENT_CLIENT_H
#define MYTFTPCLIENT_CLIENT_H

#define TFTP_PORT "69"

typedef struct TFTPClient {
	int sock;
	struct addrinfo *serv_addr;
} *client_t;

void client_free(client_t *client);
client_t client_init(char *server);

#endif//MYTFTPCLIENT_CLIENT_H