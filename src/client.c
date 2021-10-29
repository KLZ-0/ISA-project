#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "connection.h"
#include "util.h"

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
 * @param opts pointer transfer options (contents can change during the client's lifetime)
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

/**
 * Initialize the connection by opening a socket
 * @param client initialized client
 * @return EXIT_SUCCESS on success or EXIT_FAILURE on error
 */
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

	struct timeval timeout;
	timeout.tv_sec = DEFAULT_TIMEOUT;
	timeout.tv_usec = 0;

	if (setsockopt(client->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) {
		perror("SOCKOPT WARNING");
	}

	int max_block_size = find_smallest_mtu(client->sock);
	if (client->opts->block_size > max_block_size) {
		perr(TAG_CONN_INIT, "Specified block size (%lu) is larger than the smallest interface MTU (%lu)\n", client->opts->block_size, max_block_size);
		client_conn_close(client);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/**
 * Terminate the connection by closing the socket and clean up
 * @param client initialized client
 */
void client_conn_close(client_t client) {
	close(client->sock);
	freeaddrinfo(client->serv_addr);

	client->sock = -1;
	client->serv_addr = NULL;
}

/**
 * Run the client by executing a transfer request
 * @param client initialized client
 * @return EXIT_SUCCESS on success or EXIT_FAILURE on error
 */
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

/**
 * Apply the negotiated options
 * @param client initialized client
 * @param data packet data
 */
void client_apply_negotiated_opts(client_t client, char *data) {
	char *d_ptr = data;
	int got_blksize_response = 0;
	while (*d_ptr != 0) {
		if (strcmp(d_ptr, "tsize") == 0) {
			d_ptr += strlen(d_ptr) + 1;
			if (str_to_ulong(d_ptr, &client->opts->file_size) != EXIT_SUCCESS) {
				perr(TAG_CONN, "Server returned an invalid tsize value");
				client->opts->file_size = 0;
			}
		} else if (strcmp(d_ptr, "timeout") == 0) {
			// we can ignore this
			d_ptr += strlen(d_ptr) + 1;
		} else if (strcmp(d_ptr, "blksize") == 0) {
			d_ptr += strlen(d_ptr) + 1;
			if (str_to_ulong(d_ptr, &client->opts->block_size) != EXIT_SUCCESS) {
				// send an error packet also
				perr(TAG_CONN, "Server returned an invalid tsize value");
				client->opts->file_size = 0;
			}
			got_blksize_response = 1;
		}

		// move to the next option
		d_ptr += strlen(d_ptr) + 1;
	}

	// restore the block size to default if the server didn't respond to it
	if (client->opts->block_size != DEFAULT_BLOCK_SIZE && !got_blksize_response) {
		client->opts->block_size = DEFAULT_BLOCK_SIZE;
	}
}
