#include "util.h"
#include <stdlib.h>
#include <string.h>

void netascii_to_unix(char *data, size_t data_size) {
	// WARNING: Netascii messages can contain zero bytes mid-string ffs
	// \r\n -> \n
	// \r\0 -> \r
	if (strchr(data, '\r') == NULL) {
		return;
	}

	char *buffer = calloc(data_size + 1, sizeof(char));

	char *s_ptr = data;
	char *d_ptr = buffer;
	do {
		if (*s_ptr == '\r') {
			if (*++s_ptr == '\0') { // skip CR and test for CR/NUL
				s_ptr++;
				*d_ptr = '\r';
				continue;
			}
		}
		*d_ptr = *s_ptr++;
	} while (*d_ptr++ != 0);

	memcpy(data, buffer, data_size + 1);
	free(buffer);
}
