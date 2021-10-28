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
int conn_init(client_t client) {
	assert(client != NULL);

	if (client->opts->operation != OP_RRQ && client->opts->operation != OP_WRQ) {
		perr(TAG_CONN_INIT, "CONNECTION INIT ERROR: Invalid opcode %d, should be RRQ/WRQ\n", client->opts->operation);
		return EXIT_FAILURE;
	}

	// calculate message length and allocate it
	size_t msg_size = client->opts->filename_len + strlen(client->opts->mode) + 4;
	char *message = malloc(msg_size); // 2 bytes for opcode + 2x terminating null byte

	// compose the message
	char *msg_ptr = message;
	*msg_ptr++ = 0;
	*msg_ptr++ = (char)client->opts->operation;

	strcpy(msg_ptr, client->opts->filename);
	msg_ptr += client->opts->filename_len + 1;
	strcpy(msg_ptr, client->opts->mode);

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
int conn_recv_send_ack(client_t client, const char *block) {
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

	// Two buffers which we will alternate to check whether the received message is not the same as before
	// which may happen in case the ACK packet is not received by the server
	// Alternating pointer -> because then we don't need to copy the data after each received packet, we simply change the pointer
	char buf1[BUFF_SIZE] = {0};
	char buf2[BUFF_SIZE] = {0};
	char *buffer = buf1;
	char *buffer_alt = buf2;

	FILE *target_file = NULL;

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
		if (*(buffer + 1) == OP_ERROR) {
			perr(TAG_ERROR_PACKET, buffer + 4);
			goto error;
		} else if (*(buffer + 1) != OP_DATA) {
			perr(TAG_CONN, "Packet with invalid opcode received");
			goto error;
		}

		// test if the content is the same as the last received packet, including block number
		if (memcmp(buffer, buffer_alt, recvd) == 0) {
			goto resend_ack;
		}

		// open the file for writing if it is not open yet
		if (target_file == NULL) {
			target_file = fopen(client->opts->filename, "wb");
			if (target_file == NULL) {
				perror("FILE OPEN ERROR");
				goto error;
			}
		}

		// write the data to the target file
		if (client->opts->mode[0] == 'o') {
			fwrite(buffer + 4, sizeof(char), recvd - 4, target_file);
			if (ferror(target_file)) {
				perror("FILE WRITE ERROR");
				goto error;
			}
		} else if (client->opts->mode[0] == 'n') {
			size_t unix_len = netascii_to_unix(buffer + 4, recvd - 4);
			fwrite(buffer + 4, sizeof(char), unix_len, target_file);
			if (ferror(target_file)) {
				perror("FILE WRITE ERROR");
				goto error;
			}
		}

	resend_ack:
		// send ack packet
		if (conn_recv_send_ack(client, buffer + 2) != EXIT_SUCCESS) {
			goto error;
		}

		// swap buffers
		char *tmp = buffer;
		buffer = buffer_alt;
		buffer_alt = tmp;
	} while (recvd - 4 == client->opts->block_size); // TODO: Use negotiated block size

	fclose(target_file);
	return EXIT_SUCCESS;

error:
	if (target_file != NULL) {
		fclose(target_file);
	}
	return EXIT_FAILURE;
}

int conn_send_wait_for_ack(client_t client, uint16_t block_id) {
	// TODO: resend the data packet if the ACK packet is not received until timeout

	char buffer[BUFF_SIZE] = {0};

	socklen_t addr_size = sizeof(client->tid_addr);
	size_t recvd = recvfrom(client->sock, buffer, BUFF_SIZE - 1, 0, (struct sockaddr *)&client->tid_addr, &addr_size);

	if (recvd == -1) {
		perror("CONNECTION RECV ERROR");
		return EXIT_FAILURE;
	}

	if (*(buffer + 1) == OP_ERROR) {
		perr(TAG_ERROR_PACKET, buffer + 4);
		return EXIT_FAILURE;
	} else if (*(buffer + 1) != OP_ACK) {
		perr(TAG_CONN, "Packet with invalid opcode received");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
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
	if (conn_send_wait_for_ack(client, 0) != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}
	char *block = calloc(client->opts->block_size, sizeof(char)); // TODO: Use negotiated block size

	FILE *source_file = fopen(client->opts->filename, "rb");
	if (source_file == NULL) {
		perror("FILE OPEN ERROR");
		return EXIT_FAILURE;
	}

	// send content in blocks
	size_t block_bytes;
	for (size_t block_id = 1; !feof(source_file); block_id++) {
		if (client->opts->mode[0] == 'o') {
			block_bytes = fread(block, sizeof(char), client->opts->block_size, source_file); // TODO: Use negotiated block size
		} else if (client->opts->mode[0] == 'n') {
			block_bytes = file_to_netascii(source_file, block, client->opts->block_size); // TODO: Use negotiated block size
		}

		if (ferror(source_file)) {
			perror("FILE READ ERROR");
			goto error;
		}

		conn_send_block(client, block, block_bytes, block_id);
		if (conn_send_wait_for_ack(client, block_id) != EXIT_SUCCESS) {
			goto error;
		}
	}

	fclose(source_file);
	return EXIT_SUCCESS;

error:
	fclose(source_file);
	return EXIT_FAILURE;
}
