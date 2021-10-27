#include "connection.h"
#include "util.h"
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
int conn_send_init(client_t client, opcode_t opcode) {
	assert(client != NULL);

	if (opcode != OP_RRQ && opcode != OP_WRQ) {
		fprintf(stderr, "CONNECTION INIT ERROR: Invalid opcode %d, should be RRQ/WRQ\n", opcode);
		return EXIT_FAILURE;
	}

	// calculate message length and allocate it
	size_t msg_size = client->filename_len + strlen(client->mode) + 4;
	char *message = malloc(msg_size); // 2 bytes for opcode + 2x terminating null byte

	// compose the message
	char *msg_ptr = message;
	*msg_ptr++ = 0;
	*msg_ptr++ = opcode;

	strcpy(msg_ptr, client->filename);
	msg_ptr += client->filename_len + 1;
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
	FILE *target_file = fopen(client->filename, "wb");
	if (target_file == NULL) {
		perror("FILE OPEN ERROR");
		return EXIT_FAILURE;
	}

	// TODO: resend ACK in case the server sends the same data packet twice
	ssize_t recvd;
	do {
		// receive packet
		socklen_t addr_size = sizeof(client->tid_addr);
		recvd = recvfrom(client->sock, buffer, BUFF_SIZE - 1, 0, (struct sockaddr *)&client->tid_addr, &addr_size);
		if (recvd == -1) {
			perror("CONNECTION RECV ERROR");
			goto error;
		}
		buffer[recvd] = 0;

		// process received packet
		if (*(buffer + 1) != OP_DATA) {
			// TODO: not a data packet
			goto error;
		}

		// send ack packet
		if (conn_recv_ack(client, buffer + 2) != EXIT_SUCCESS) {
			goto error;
		}

		// write the data to the target file
		if (client->mode[0] == 'o') { // binary
			fwrite(buffer + 4, sizeof(char), recvd - 4, target_file);
			if (ferror(target_file)) {
				perror("FILE WRITE ERROR");
				goto error;
			}
		} else if (client->mode[0] == 'n') { // netascii
			size_t unix_len = netascii_to_unix(buffer + 4, recvd - 4);
			fwrite(buffer + 4, sizeof(char), unix_len, target_file);
			if (ferror(target_file)) {
				perror("FILE WRITE ERROR");
				goto error;
			}
		}
	} while (recvd - 4 == BLOCK_SIZE);

	fclose(target_file);
	return EXIT_SUCCESS;

error:
	fclose(target_file);
	return EXIT_FAILURE;
}

void conn_send_wait_for_ack(client_t client, uint16_t block_id) {
	char buffer[BUFF_SIZE] = {0};

	socklen_t addr_size = sizeof(client->tid_addr);
	size_t recvd = recvfrom(client->sock, buffer, BUFF_SIZE - 1, 0, (struct sockaddr *)&client->tid_addr, &addr_size);
}

int conn_send_block(client_t client, char *block, size_t block_size, size_t block_id) {
	char *message = calloc(block_size + 4, sizeof(char));

	message[0] = 0;
	message[1] = OP_DATA;
	message[2] = (char)((block_id & 0xFF00) >> 8);
	message[3] = (char)(block_id & 0xFF);

	memcpy(message + 4, block, block_size);

	socklen_t addr_size = sizeof(client->tid_addr);
	sendto(client->sock, message, block_size + 4, 0, (struct sockaddr *)&client->tid_addr, addr_size);

	free(message);
	return EXIT_SUCCESS;
}

int conn_send(client_t client) {
	conn_send_wait_for_ack(client, 0);
	char *block = calloc(client->block_size, sizeof(char));

	FILE *source_file = fopen(client->filename, "rb");

	// send content in blocks
	size_t block_bytes;
	for (size_t block_id = 1; !feof(source_file); block_id++) {
		block_bytes = fread(block, sizeof(char), client->block_size, source_file);
		if (ferror(source_file)) {
			perror("FILE READ ERROR");
			goto error;
		}
		conn_send_block(client, block, block_bytes, block_id);
		conn_send_wait_for_ack(client, block_id);
	}

	fclose(source_file);
	return EXIT_SUCCESS;

error:
	fclose(source_file);
	return EXIT_FAILURE;
}
