#include "util.h"
#include <limits.h>
#include <net/if.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>

/**
 * Convert data from netascii to unix format
 * @param data input data to be modified
 * @param data_size input data size
 * @return resulting data size (less or equal to data_size)
 */
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

/**
 * Read an unspecified number of bytes from a file
 * so that the resulting block will have block_size bytes in netascii format
 * @param file input file
 * @param block output block buffer
 * @param block_size output block buffer size
 * @return number of bytes in the output buffer (less or equal to block_size)
 */
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
	if (input == NULL || argv == NULL) {
		return ARGC_ERROR;
	}

	// clear argv
	memset(argv, 0, max_argc);

	// program name - we should emulate the real argv
	argv[0] = PROG_NAME;
	int argc = 1;
	if (input[0] == '\n') {
		return argc;
	}

	// first word
	argv[argc++] = input;

	// rest of the input
	char *sp_ptr = input;
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
			return ARGC_ERROR;
		}
	}

	// remove trailing newline
	argv[argc - 1][strlen(argv[argc - 1]) - 1] = '\0';

	return argc;
}

/**
 * Wrapper around formatted print to stderr
 * @param tag Application part
 * @param fmt Message
 * @param ... format
 */
void perr(const char *tag, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "%s: ", tag);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
}

void get_time(char *buffer) {
	char format_buffer[BUFF_SIZE];

	// seconds + milliseconds
	struct timeval now;
	gettimeofday(&now, NULL);

	// format string generation
	struct tm *timestruct = localtime(&now.tv_sec);
	strftime(format_buffer, BUFF_SIZE, "%F %T.%%li", timestruct);

	// add the milliseconds
	sprintf(buffer, format_buffer, now.tv_usec / 1000);
}

/**
 * Wrapper around formatted print to stdout
 * @param fmt Message
 * @param ... format
 */
void pinfo(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	char buf[BUFF_SIZE];
	get_time(buf);
	printf("[%s] ", buf);

	vprintf(fmt, args);
	fputc('\n', stdout);
	fflush(stdout);
}

/**
 * Wrapper around formatted print to stdout without the ending newline
 * @param fmt Message
 * @param ... format
 */
void pinfo_cont(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	char buf[BUFF_SIZE];
	get_time(buf);
	printf("[%s] ", buf);

	vprintf(fmt, args);
}

/**
 * Convert the source string to unsigned long and store it in target
 * @param source source string
 * @param target pointer to an unsigned long where the value should be stored
 * @return EXIT_SUCCESS on success or EXIT_FAILURE on error
 */
int str_to_ulong(const char *source, unsigned long *target) {
	char *endptr;

	unsigned long tmp = strtoul(source, &endptr, 0);
	if (*endptr != '\0') {
		return EXIT_FAILURE;
	}

	*target = tmp;
	return EXIT_SUCCESS;
}

/**
 * Find the smallest device MTU
 * @param sock open socket
 * @return smallest MTU
 */
int find_smallest_mtu(int sock) {
	int min_mtu = INT_MAX;

	struct ifreq tmp;
	for (int i = 1; i < MAX_INTERFACES; ++i) {
		tmp.ifr_ifindex = i;

		// get interface name from index
		if (ioctl(sock, SIOCGIFNAME, &tmp) == -1) {
			// we didn't find any other interfaces
			// device with index 1 is lo, so this should never return INT_MAX
			return min_mtu;
		}

		// get MTU size
		if (ioctl(sock, SIOCGIFMTU, &tmp) == -1) {
			perror("IOCTL WARNING");
			return min_mtu;
		}

		// choose the biggest one
		if (tmp.ifr_mtu < min_mtu) {
			min_mtu = tmp.ifr_mtu;
		}
	}

	// this should also never happen, but can happen if the user has 255+ interfaces
	// this is just a safeguard in case ioctl would not want to error out
	// if the user really has 255+ interfaces either change the value of MAX_INTERFACES
	// or just deal with a possibly bigger MTU which still shouldn't affect much
	return min_mtu;
}
