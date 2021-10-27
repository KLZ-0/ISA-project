#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "client.h"
#include "connection.h"


int client_read(client_t client, char *filename) {
	if (conn_send_init(client, filename, OP_RRQ) != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	if (conn_recv(client) != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int main() {
//	client_t client = client_init("localhost");
//	client_t client = client_init("127.0.0.1");
	client_t client = client_init("::1");
	if (client == NULL) {
		return EXIT_FAILURE;
	}

	if (client_read(client, "lorem") != EXIT_SUCCESS) {
		client_free(&client);
		return EXIT_FAILURE;
	}

	client_free(&client);
	return EXIT_SUCCESS;
}
