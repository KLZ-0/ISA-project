#include "util.h"
#include <stdlib.h>
#include <string.h>

size_t netascii_to_unix(char *data, size_t data_size) {
	// WARNING: Netascii messages can contain zero bytes mid-string ffs
	// \r\n -> \n
	// \r\0 -> \r
	if (strchr(data, '\r') == NULL) {
		return data_size;
	}

	char *buffer = calloc(data_size + 1, sizeof(char));

	char *d_ptr = buffer;
	size_t newsize = 0;

	for (size_t i = 0; i < data_size; i++) {
		if (data[i] == '\r') {
			if (data[++i] == '\0') {
				data[i] = '\r';
			}
		}

		newsize++;
		*d_ptr++ = data[i];
	}

	memcpy(data, buffer, newsize + 1);
	free(buffer);

	return newsize;
}
