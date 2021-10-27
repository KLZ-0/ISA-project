#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"

/**
 * Free the memory allocated by the TFTPv2 client
 * @param client initialized client or NULL
 */
void client_free(client_t *client) {
	if (client == NULL || *client == NULL) {
		return;
	}

	close((*client)->sock);
	freeaddrinfo((*client)->serv_addr);

	free(*client);
	*client = NULL;
}

/**
 * Initialize the TFTPv2 client structure
 * @param server target server to connect
 * @return initialized client_t or NULL on error
 */
client_t client_init(char *server) {
	struct addrinfo hints = {0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	client_t client = calloc(1, sizeof(struct TFTPClient));
	if (client == NULL) {
		perror("CLIENT INIT ALLOC ERROR");
		return NULL;
	}

	if (getaddrinfo(server, TFTP_PORT, &hints, &client->serv_addr)) {
		perror("CLIENT INIT ADDRESS ERROR");
		goto error;
	}

	client->sock = socket(client->serv_addr->ai_family, client->serv_addr->ai_socktype, client->serv_addr->ai_protocol);
	if (client->sock == -1) {
		perror("CLIENT INIT SOCKET ERROR");
		goto error;
	}

	client->mode = "octet";

	return client;

error:
	client_free(&client);
	return NULL;
}
