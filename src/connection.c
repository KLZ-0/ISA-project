#include "connection.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void conn_init_msg_append_option(char *message, size_t *shift, char *option, size_t value) {
	strcpy(message + *shift, option);
	*shift += strlen(message + *shift) + 1;

	sprintf(message + *shift, "%lu", value);
	*shift += strlen(message + *shift) + 1;
}

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

	// calculate message length
	// 2 bytes for opcode + 2x terminating null byte + possible options
	size_t msg_size = 0;
	char message[BUFF_SIZE] = {0};

	// compose the message
	message[msg_size++] = 0;
	message[msg_size++] = (char)client->opts->operation;

	// filename
	strcpy(message + msg_size, client->opts->filename_abs);
	msg_size += client->opts->filename_abs_len + 1;
	strcpy(message + msg_size, client->opts->mode);
	msg_size += strlen(message + msg_size) + 1;

	// timeout
	if (client->opts->timeout > 0) {
		conn_init_msg_append_option(message, &msg_size, "timeout", client->opts->timeout);
	}

	// tsize
	conn_init_msg_append_option(message, &msg_size, "tsize", client->opts->file_size);

	// blksize
	conn_init_msg_append_option(message, &msg_size, "blksize", client->opts->block_size);

	// send the message
	pinfo("Requesting %s from server %s:%s", (client->opts->operation == OP_WRQ) ? "WRITE" : "READ", client->opts->raw_addr, client->opts->raw_port);
	ssize_t sent = sendto(client->sock, message, msg_size, 0, client->serv_addr->ai_addr, client->serv_addr->ai_addrlen);
	if (sent == -1) {
		perror("CONNECTION INIT ERROR");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/**
 * Send and acknowledgement packet for the received block
 * @param client initialized client
 * @param block block number to acknowledge (2 byte word, big endian)
 * @return EXIT_SUCCESS on success or EXIT_FAILURE on error
 */
int conn_send_ack(client_t client, const char *block) {
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
	char buf1[DEFAULT_BLOCK_SIZE + 4] = {0};
	char buf2[DEFAULT_BLOCK_SIZE + 4] = {0};
	char *buffer = buf1;
	char *buffer_alt = buf2;
	int buffer_allocd = 0;

	FILE *target_file = NULL;

	uint16_t block_id = 0;
	ssize_t recvd;
	ssize_t recvd_total = 0;
	do {
		// receive packet
		socklen_t addr_size = sizeof(client->tid_addr);
		recvd = recvfrom(client->sock, buffer, client->opts->block_size + 4, 0, (struct sockaddr *)&client->tid_addr, &addr_size);
		if (recvd == -1) {
			perror("Server cannot be reached");
			goto error;
		}

		// process received packet
		switch (*(buffer + 1)) {
			case OP_DATA:
				if (block_id++ == 0 && buffer_allocd == 0) {
					// the server ignored the options and sent the data -> continue with the default block size
					client->opts->block_size = DEFAULT_BLOCK_SIZE;
				}
				break;
			case OP_ERROR:
				perr(TAG_ERROR_PACKET, buffer + 4);
				goto error;
			case OP_OPTACK:
				client_apply_negotiated_opts(client, buffer + 2, recvd - 2);
				buffer = malloc(client->opts->block_size);
				buffer_alt = malloc(client->opts->block_size);
				buffer_allocd = 1;
				conn_send_ack(client, "\0");
				recvd = (long)client->opts->block_size + 4;
				continue;
			default:
				perr(TAG_CONN, "Packet with invalid opcode received");
				goto error;
		}

		// test if the content is the same as the last received packet, including block number
		if (memcmp(buffer, buffer_alt, recvd) == 0) {
			pinfo("Received duplicated DATA packet... resending ACK for previous block");
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

		// inform the user
		pinfo_cont("Receiving DATA... %lu B", recvd_total += recvd - 4);
		if (client->opts->file_size) {
			printf(" of %lu B\n", client->opts->file_size);
		} else {
			printf("\n");
		}

	resend_ack:
		// send ack packet
		if (conn_send_ack(client, buffer + 2) != EXIT_SUCCESS) {
			goto error;
		}

		// swap buffers
		char *tmp = buffer;
		buffer = buffer_alt;
		buffer_alt = tmp;
	} while (recvd - 4 == client->opts->block_size);

	fclose(target_file);
	return EXIT_SUCCESS;

error:
	if (target_file != NULL) {
		fclose(target_file);
	}
	if (buffer_allocd) {
		free(buffer);
		free(buffer_alt);
	}
	return EXIT_FAILURE;
}

int conn_send_wait_for_ack(client_t client, uint16_t block_id) {
	char buffer[BUFF_SIZE] = {0};

	socklen_t addr_size = sizeof(client->tid_addr);
	size_t recvd = recvfrom(client->sock, buffer, BUFF_SIZE - 1, 0, (struct sockaddr *)&client->tid_addr, &addr_size);

	if (recvd == -1) {
		return EXIT_RETRY;
	}

	switch (*(buffer + 1)) {
		case OP_ACK:
			if (block_id == 0) {
				// the server ignored the options and sent the data -> continue with the default block size
				client->opts->block_size = DEFAULT_BLOCK_SIZE;
			}
			break;
		case OP_ERROR:
			perr(TAG_ERROR_PACKET, buffer + 4);
			return EXIT_FAILURE;
		case OP_OPTACK:
			client_apply_negotiated_opts(client, buffer + 2, recvd - 2);
			break;
		default:
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
	// set timeout for ack packets and wait for one, if not received retry the whole connection initialization
	int response = conn_send_wait_for_ack(client, 0);
	if (response == EXIT_RETRY) {
		perror("Server cannot be reached");
	}
	if (response != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	// try to open the file for reading
	FILE *source_file = fopen(client->opts->filename, "rb");
	if (source_file == NULL) {
		perror("FILE OPEN ERROR");
		return EXIT_FAILURE;
	}

	// send content in blocks
	char *block = calloc(client->opts->block_size, sizeof(char));
	size_t block_bytes;
	size_t sent_total = 0;
	for (size_t block_id = 1; !feof(source_file); block_id++) {
		// copy the block into the buffer
		if (client->opts->mode[0] == 'o') {
			block_bytes = fread(block, sizeof(char), client->opts->block_size, source_file);
		} else if (client->opts->mode[0] == 'n') {
			block_bytes = file_to_netascii(source_file, block, client->opts->block_size);
		}
		sent_total += block_bytes;

		// exit on error
		if (ferror(source_file)) {
			perror("FILE READ ERROR");
			goto error;
		}

		int resend_count = 0;

	resend_block:
		// inform the user
		pinfo_cont("Sending DATA... %lu B", sent_total);
		if (client->opts->file_size) {
			printf(" of %lu B\n", client->opts->file_size);
		} else {
			printf("\n");
		}

		// send the block
		conn_send_block(client, block, block_bytes, block_id);

		// wait for ACK
		if ((response = conn_send_wait_for_ack(client, block_id)) != EXIT_SUCCESS) {
			if (resend_count++ < RESEND_COUNT_MAX && response == EXIT_RETRY) {
				pinfo("ACK packet not received, resending DATA packet");
				goto resend_block;
			}
			goto error;
		}
	}

	fclose(source_file);
	return EXIT_SUCCESS;

error:
	fclose(source_file);
	return EXIT_FAILURE;
}
