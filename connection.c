#include "connection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int conn_send_rrq(client_t client, char *filename) {
	size_t msg_size = strlen(filename) + strlen(client->mode) + 4;
	char *message = malloc(msg_size); // 2 bytes for opcode + 2x terminating null byte

	char *msg_ptr = message;
	*msg_ptr++ = 0;
	*msg_ptr++ = 1;

	strcpy(msg_ptr, filename);
	msg_ptr += strlen(filename) + 1;

	strcpy(msg_ptr, client->mode);

	ssize_t sent = sendto(client->sock, message, msg_size, 0, client->serv_addr->ai_addr, client->serv_addr->ai_addrlen);
	if (sent == -1) {
		free(message);
		return EXIT_FAILURE;
	}
	printf("RRQ: sent %lu bytes to the server\n", sent);

	free(message);
	return EXIT_SUCCESS;
}

int conn_recv_ack(client_t client, const char *block) {
	size_t ack_size = 4;
	char ack_msg[ack_size];

	ack_msg[0] = 0;
	ack_msg[1] = 4;
	ack_msg[2] = block[0];
	ack_msg[3] = block[1];

	printf("sending ACK...\n");
	socklen_t addr_size = sizeof(client->tid_addr);
	size_t sent = sendto(client->sock, ack_msg, ack_size, 0, (struct sockaddr*) &client->tid_addr, addr_size);
	if (sent == -1) {
		return EXIT_FAILURE;
	}
	printf("sent %lu bytes to the server\n", sent);

	return EXIT_SUCCESS;
}

int conn_recv(client_t client) {
	char buffer[BUFF_SIZE] = {0};

	socklen_t addr_size = sizeof(client->tid_addr);
	ssize_t recvd = recvfrom(client->sock, buffer, BUFF_SIZE - 1, 0, (struct sockaddr*) &client->tid_addr, &addr_size);
	if (recvd == -1) {
		return EXIT_FAILURE;
	}
	printf("RECV: received %lu bytes from the server\n", recvd);

	// process received packet
	if (*(buffer + 1) != 3) {
		printf("Size %d\n", *(buffer + 1));
		return EXIT_FAILURE;
	}

	uint16_t block_number = *(buffer + 3) + (*(buffer + 2) << 8);
	printf("DATA (block %d): %s\n", block_number, buffer + 4);

	if (conn_recv_ack(client, buffer + 2) != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
