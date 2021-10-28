#ifndef MYTFTPCLIENT_UTIL_H
#define MYTFTPCLIENT_UTIL_H

#include <stdio.h>

#define BUFF_SIZE 1024

size_t netascii_to_unix(char *data, size_t data_size);
size_t file_to_netascii(FILE *file, char *block, size_t block_size);

#endif //MYTFTPCLIENT_UTIL_H
