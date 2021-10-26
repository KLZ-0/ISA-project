#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "client.h"

#define BUFF_SIZE 1024
char buffer[BUFF_SIZE];

int client_read(client_t client, char *filename) {
	char *mode = "netascii";

	size_t msg_size = strlen(filename) + strlen(mode) + 4;
	char *message = malloc(msg_size); // 2 bytes for opcode + 2x terminating null byte

	char *msg_ptr = message;
	*msg_ptr++ = 0;
	*msg_ptr++ = 1;

	strcpy(msg_ptr, filename);
	msg_ptr += strlen(filename) + 1;

	strcpy(msg_ptr, mode);

	ssize_t sent = sendto(client->sock, message, msg_size, 0, client->serv_addr->ai_addr, client->serv_addr->ai_addrlen);
	if (sent == -1) {
		return EXIT_FAILURE;
	}
	printf("sent %lu bytes to the server\n", sent);

	struct sockaddr_storage tid_addr;
	socklen_t addr_size = sizeof(tid_addr);
	ssize_t recvd = recvfrom(client->sock, buffer, BUFF_SIZE - 1, 0, (struct sockaddr*) &tid_addr, &addr_size);
	if (recvd == -1) {
		return EXIT_FAILURE;
	}
	printf("received %lu bytes from the server\n", sent);

	// process received packet
	if (*(buffer + 1) != 3) {
		printf("Size %d\n", *(buffer + 1));
		return EXIT_FAILURE;
	}

	uint16_t block_number = *((uint16_t *)(buffer + 2));
	printf("DATA (block %d): %s\n", block_number, buffer + 4);

	size_t ack_size = 4;
	char ack_msg[ack_size];

	ack_msg[0] = 0;
	ack_msg[1] = 4;
	ack_msg[2] = buffer[2];
	ack_msg[3] = buffer[3];

	printf("sending ACK...\n");
	sent = sendto(client->sock, ack_msg, ack_size, 0, (struct sockaddr*) &tid_addr, addr_size);
	if (sent == -1) {
		return EXIT_FAILURE;
	}
	printf("sent %lu bytes to the server\n", sent);

	free(message);
	return EXIT_SUCCESS;
}

int main() {
//	client_t client = client_init("localhost");
//	client_t client = client_init("127.0.0.1");
	client_t client = client_init("::1");
	if (client == NULL) {
		return EXIT_FAILURE;
	}

	client_read(client, "test");

	client_free(&client);
	return EXIT_SUCCESS;
}
