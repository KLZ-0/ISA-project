#ifndef MYTFTPCLIENT_ARGS_H
#define MYTFTPCLIENT_ARGS_H

#include <stdlib.h>

#define DEFAULT_BLOCK_SIZE 512

/**
 * Program options
 */
typedef struct ProgramOptions {
	int operation;           ///< mode of operation ( OP_RRQ | OP_WRQ )
	char *filename_abs;      ///< absolute filename including the path on the server
	size_t filename_abs_len; ///< length of filename_abs (so that we don't have to strlen each time)
	char *filename;          ///< filename without the path where our local file is located
	size_t file_size;        ///< file size in bytes
	size_t timeout;          ///< server resend timeout
	size_t block_size;       ///< block size to be negotiated
	int multicast;           ///< multicast mode
	char *mode;              ///< transmission mode ("netascii" or "octet")
	char *raw_addr;          ///< target address in string format
	char *raw_port;          ///< target port in string format
} options_t;

int parse_options(int argc, char *argv[], options_t *opts);

#endif //MYTFTPCLIENT_ARGS_H
