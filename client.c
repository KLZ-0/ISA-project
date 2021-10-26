#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"

void client_free(client_t *client) {
	if (client == NULL || *client == NULL) {
		return;
	}

	close((*client)->sock);
	freeaddrinfo((*client)->serv_addr);

	free(*client);
	*client = NULL;
}

client_t client_init(char *server) {
	struct addrinfo hints = {0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	client_t client = calloc(1, sizeof(struct TFTPClient));
	if (client == NULL) {
		return NULL;
	}

	if (getaddrinfo(server, TFTP_PORT, &hints, &client->serv_addr)) {
		goto error;
	}

	client->sock = socket(client->serv_addr->ai_family, client->serv_addr->ai_socktype, client->serv_addr->ai_protocol);
	if (client->sock == -1) {
		goto error;
	}

	return client;

error:
	client_free(&client);
	return NULL;
}