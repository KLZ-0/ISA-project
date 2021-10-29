#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "connection.h"

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
client_t client_init(options_t *opts) {
	client_t client = calloc(1, sizeof(struct TFTPClient));
	if (client == NULL) {
		perror("CLIENT INIT ALLOC ERROR");
		return NULL;
	}

	client->opts = opts;

	return client;
}

int client_conn_init(client_t client) {
	struct addrinfo hints = {0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if (getaddrinfo(client->opts->raw_addr, client->opts->raw_port, &hints, &client->serv_addr)) {
		perror("CLIENT INIT ADDRESS ERROR");
		client->serv_addr = NULL;
		return EXIT_FAILURE;
	}

	client->sock = socket(client->serv_addr->ai_family, client->serv_addr->ai_socktype, client->serv_addr->ai_protocol);
	if (client->sock == -1) {
		perror("CLIENT INIT SOCKET ERROR");
		client->serv_addr = NULL;
		client->sock = -1;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void client_conn_close(client_t client) {
	close(client->sock);
	freeaddrinfo(client->serv_addr);

	client->sock = -1;
	client->serv_addr = NULL;
}

int client_run(client_t client) {
	if (conn_init(client) != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	if (client->opts->operation == OP_WRQ) {
		return conn_send(client);
	} else {
		return conn_recv(client);
	}
}

void client_set_timeout(client_t client) {
	struct timeval timeout;
	timeout.tv_sec = (time_t)client->opts->timeout;
	timeout.tv_usec = 0;

	if (setsockopt(client->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) {
		perror("SOCKOPT WARNING");
	}
}

void client_reset_timeout(client_t client) {
	struct timeval timeout;
	timeout.tv_sec = DEFAULT_TIMEOUT;
	timeout.tv_usec = 0;

	if (setsockopt(client->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) {
		perror("SOCKOPT WARNING");
	}
}
