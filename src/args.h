#ifndef MYTFTPCLIENT_ARGS_H
#define MYTFTPCLIENT_ARGS_H

#include <stdlib.h>

#define DEFAULT_BLOCK_SIZE 512

typedef struct ProgramOptions {
	int operation;
	char *filename;
	size_t filename_len;
	size_t timeout;
	size_t block_size;
	int multicast;
	char *mode; ///< transmission mode ("netascii" or "octet")
	char *raw_addr;
	char *raw_port;
} options_t;

int parse_options(int argc, char *argv[], options_t *opts);

#endif //MYTFTPCLIENT_ARGS_H
