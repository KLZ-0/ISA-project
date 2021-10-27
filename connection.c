#include "connection.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Establish the connection by sending an RRQ/WRQ packet
 * @param client initialized client
 * @param filename file to get from the server
 * @return EXIT_SUCCESS on success or EXIT_FAILURE on error
 */
int conn_send_init(client_t client, char *filename, opcode_t opcode) {
	assert(client != NULL);
	assert(filename != NULL);

	if (opcode != OP_RRQ && opcode != OP_WRQ) {
		fprintf(stderr, "CONNECTION INIT ERROR: Invalid opcode %d, should be RRQ/WRQ\n", opcode);
		return EXIT_FAILURE;
	}

	// calculate message length and allocate it
	size_t msg_size = strlen(filename) + strlen(client->mode) + 4;
	char *message = malloc(msg_size); // 2 bytes for opcode + 2x terminating null byte

	// compose the message
	char *msg_ptr = message;
	*msg_ptr++ = 0;
	*msg_ptr++ = opcode;

	strcpy(msg_ptr, filename);
	msg_ptr += strlen(filename) + 1;
	strcpy(msg_ptr, client->mode);

	// send the message
	ssize_t sent = sendto(client->sock, message, msg_size, 0, client->serv_addr->ai_addr, client->serv_addr->ai_addrlen);
	if (sent == -1) {
		perror("CONNECTION INIT ERROR");
		free(message);
		return EXIT_FAILURE;
	}

	// end
	free(message);
	return EXIT_SUCCESS;
}

/**
 * Send and acknowledgement packet for the received block
 * @param client initialized client
 * @param block block number to acknowledge (2 byte word, big endian)
 * @return EXIT_SUCCESS on success or EXIT_FAILURE on error
 */
int conn_recv_ack(client_t client, const char *block) {
	assert(client != NULL);
	assert(block != NULL);

	// compose the message
	char ack_msg[ACK_MSG_SIZE];

	ack_msg[0] = 0;
	ack_msg[1] = OP_ACK;
	ack_msg[2] = block[0];
	ack_msg[3] = block[1];

	// send the message
	socklen_t addr_size = sizeof(client->tid_addr);
	size_t sent = sendto(client->sock, ack_msg, ACK_MSG_SIZE, 0, (struct sockaddr *)&client->tid_addr, addr_size);
	if (sent == -1) {
		perror("CONNECTION RECV ACK ERROR");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/**
 * Receive a file from the server
 * @param client initialized client
 * @return EXIT_SUCCESS on success or EXIT_FAILURE on error
 */
int conn_recv(client_t client) {
	assert(client != NULL);

	char buffer[BUFF_SIZE] = {0};

	ssize_t recvd;
	do {
		// receive packet
		socklen_t addr_size = sizeof(client->tid_addr);
		recvd = recvfrom(client->sock, buffer, BUFF_SIZE - 1, 0, (struct sockaddr *)&client->tid_addr, &addr_size);
		if (recvd == -1) {
			perror("CONNECTION RECV ERROR");
			return EXIT_FAILURE;
		}

		// process received packet
		if (*(buffer + 1) != OP_DATA) {
			// TODO: not a data packet
			return EXIT_FAILURE;
		}

		uint16_t block_number = *(buffer + 3) + (*(buffer + 2) << 8);
		printf("DATA (block %d): %s\n", block_number, buffer + 4);

		// send ack packet
		if (conn_recv_ack(client, buffer + 2) != EXIT_SUCCESS) {
			return EXIT_FAILURE;
		}
	} while (recvd - 4 == BLOCK_SIZE);

	return EXIT_SUCCESS;
}