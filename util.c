#include "util.h"
#include <stdlib.h>
#include <string.h>

size_t netascii_to_unix(char *data, size_t data_size) {
	// WARNING: Netascii messages can contain zero bytes mid-string ffs
	// \r\n -> \n
	// \r\0 -> \r
	// lastchar is needed to identify if tese happen between UDP messages
	// 00007690 -> E..N
	// 09 08 8c 09 61 0b 45 0d  4e 0f 6f 11 d7 13 9e 14
	// 09 08 8c 09 61 0b 45 0d  00 4e 0f 6f 11 d7 13 9e
	// 2f fe 53 ff 44 00 01 01 33 01
	// 2f fe 53 ff 44 01 01 33 01
	// 0000a200
	// 0a fa 84 fa 38 fb 23 fc  b7 fc 41 fd 63 fd 3b fd
	// 0d 0a fa 84 fa 38 fb 23  fc b7 fc 41 fd 63 fd 3b

	// inc /r | /n

	char tmplastchar = 0;
	static char lastchar = 0;

	char buffer[BUFF_SIZE] = {0};

	char *d_ptr = buffer;
	size_t newsize = 0;

	for (size_t i = 0; i < data_size; i++) {
		if (data[i] == '\r') {
			if (++i == data_size) {
				tmplastchar = '\r';
				break;
			}
			if (data[i] == '\0') {
				data[i] = '\r';
			}
		} else if (i == 0 && lastchar == '\r') {
			if (data[0] == 0) {
				data[0] = '\r';
			}
		}

		newsize++;
		*d_ptr++ = data[i];
		tmplastchar = data[i];
	}

	memcpy(data, buffer, newsize + 1);

	lastchar = tmplastchar;
	return newsize;
}
