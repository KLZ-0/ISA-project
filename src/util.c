#include "util.h"
#include <stdlib.h>
#include <string.h>

size_t netascii_to_unix(char *data, size_t data_size) {
	// WARNING: Netascii messages can contain zero bytes mid-string ffs
	// \r\n -> \n
	// \r\0 -> \r
	// lastchar is needed to identify if these happen between UDP messages

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

size_t file_to_netascii(FILE *file, char *block, size_t block_size) {
	static char lastchar = 0;

	int c;
	for (int i = 0; i < block_size; ++i) {
		if (i == 0 && lastchar != 0) {
			if (lastchar == '\r') {
				lastchar = '\0';
			}
			block[i++] = lastchar;
			lastchar = 0;
		}

		if ((c = fgetc(file)) == EOF) {
			return i;
		}

		if (ferror(file)) {
			perror("FILE READ ERROR");
			return i;
		}

		block[i] = (char)c;

		if (c == '\n' || c == '\r') {
			block[i++] = '\r';

			if (i == block_size) {
				lastchar = (char)c;
				c = 0; // hack but works
			}
		}

		switch (c) {
			case '\n':
				block[i] = '\n';
				break;
			case '\r':
				block[i] = '\0';
				break;
			default:
				break;
		}
	}

	return block_size;
}

/**
 * Make argc & argv from a given input string
 * @param input zero terminated input string
 * @param argv list of char* to store beginnings of words
 * @param max_argc maximum number of args
 * @return argc
 */
int make_argv(char *input, char *argv[], size_t max_argc) {
	// first word
	char *sp_ptr = input;
	argv[0] = input;

	// rest of the input
	int argc = 1;
	while ((sp_ptr = strchr(sp_ptr, ' ')) != NULL) {
		// replace space with zero byte
		*sp_ptr++ = '\0';

		// skip multiple spaces
		if (*sp_ptr == ' ') {
			continue;
		}

		// store word pointer, return in case the next would exceed memory size
		argv[argc++] = sp_ptr;
		if (argc == max_argc) {
			return argc;
		}
	}

	// remove trailing newline
	argv[argc - 1][strlen(argv[argc - 1]) - 1] = '\0';

	return argc;
}
