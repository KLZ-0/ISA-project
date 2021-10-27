#ifndef MYTFTPCLIENT_UTIL_H
#define MYTFTPCLIENT_UTIL_H

#include <stdio.h>

#define BUFF_SIZE 1024

size_t netascii_to_unix(char *data, size_t data_size);
size_t unix_to_netascii(char *data);

#endif //MYTFTPCLIENT_UTIL_H
