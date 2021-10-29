#ifndef MYTFTPCLIENT_CLIENT_H
#define MYTFTPCLIENT_CLIENT_H

#include "args.h"
#include <netdb.h>

#define EXIT_RETRY 3

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
	struct sockaddr_storage tid_addr; ///< Server address with the server's chosen TID port (acquired after first received packet)
	options_t *opts;
} * client_t;

void client_free(client_t *client);
client_t client_init(options_t *opts);
int client_conn_init(client_t client);
void client_conn_close(client_t client);
int client_run(client_t client);
void client_set_timeout(client_t client);
void client_reset_timeout(client_t client);

#endif //MYTFTPCLIENT_CLIENT_H
