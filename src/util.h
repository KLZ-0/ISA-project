#ifndef MYTFTPCLIENT_UTIL_H
#define MYTFTPCLIENT_UTIL_H

#include <stdio.h>

#define PROG_NAME "myftpclient"
#define BUFF_SIZE 1024
#define ARGC_ERROR -1

#define TAG_ARGSPARSE "ARGUMENT PARSER ERROR"
#define TAG_INPUT "INPUT ERROR"
#define TAG_CONN_INIT "CONNECTION INIT ERROR"
#define TAG_CONN "CONNECTION ERROR"
#define TAG_ERROR_PACKET "RECEIVED ERROR PACKET"

size_t netascii_to_unix(char *data, size_t data_size);
size_t file_to_netascii(FILE *file, char *block, size_t block_size);
int make_argv(char *input, char *argv[], size_t max_argc);
void perr(const char *tag, const char *fmt, ...);
void pinfo(const char *fmt, ...);

#endif //MYTFTPCLIENT_UTIL_H
