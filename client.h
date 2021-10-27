#ifndef MYTFTPCLIENT_CLIENT_H
#define MYTFTPCLIENT_CLIENT_H

#include <netdb.h>

#define TFTP_PORT "69"

typedef enum Opcode
{
	OP_RRQ = 1,
	OP_WRQ = 2,
	OP_DATA = 3,
	OP_ACK = 4,
	OP_ERROR = 5,
} opcode_t;

/**
 * TFTPv2 client structure
 */
typedef struct TFTPClient {
	int sock;                         ///< Connection socket
	struct addrinfo *serv_addr;       ///< Server address with the inital TID port (69 by default)
	char *mode;                       ///< transmission mode ("netascii" or "octet")
	struct sockaddr_storage tid_addr; ///< Server address with the server's chosen TID port (acquired after first received packet)
} * client_t;

void client_free(client_t *client);
client_t client_init(char *server);

#endif //MYTFTPCLIENT_CLIENT_H
