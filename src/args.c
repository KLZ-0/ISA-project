#include "args.h"
#include "client.h"
#include "util.h"
#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * Convert optarg to unsigned long and store it in target
 * @param target pointer to an unsigned long where the value should be stored
 * @return EXIT_SUCCESS on success or EXIT_FAILURE on error
 */
int optarg_to_ulong(unsigned long *target) {
	char *endptr;

	unsigned long tmp = strtoul(optarg, &endptr, 0);
	if (*endptr != '\0') {
		return EXIT_FAILURE;
	}

	*target = tmp;
	return EXIT_SUCCESS;
}

int parse_options(int argc, char *argv[], options_t *opts) {
	memset(opts, 0, sizeof(struct ProgramOptions));
	opts->mode = "octet";
	opts->block_size = DEFAULT_BLOCK_SIZE;
	opts->raw_addr = "127.0.0.1";
	opts->raw_port = "69";

	opterr = 0;
	optind = 1;

	int c;
	while ((c = getopt(argc, argv, "RWd:t:s:mc:a:")) != -1) {
		switch (c) {
			case 'R':
				if (opts->operation != 0) {
					return EXIT_FAILURE;
				}
				opts->operation = OP_RRQ;
				break;
			case 'W':
				if (opts->operation != 0) {
					return EXIT_FAILURE;
				}
				opts->operation = OP_WRQ;
				break;
			case 'd':
				opts->filename = optarg;
				opts->filename_len = strlen(optarg);
				break;
			case 't':
				if (optarg_to_ulong(&opts->timeout) != EXIT_SUCCESS) {
					perr(TAG_ARGSPARSE, "Timeout not a valid number");
					return EXIT_FAILURE;
				}
				break;
			case 's':
				if (optarg_to_ulong(&opts->block_size) != EXIT_SUCCESS) {
					perr(TAG_ARGSPARSE, "Block size not a valid number");
					return EXIT_FAILURE;
				}
				break;
			case 'm':
				opts->multicast = 1;
				break;
			case 'c':
				if (strcmp(optarg, "binary") == 0 || strcmp(optarg, "octet") == 0) {
					break;
				} else if (strcmp(optarg, "ascii") == 0 || strcmp(optarg, "netascii") == 0) {
					opts->mode = "netascii";
				} else {
					perr(TAG_ARGSPARSE, "Mode '%s' not one of ( binary | octet | ascii | netascii )", optarg);
					return EXIT_FAILURE;
				}
				break;
			case 'a':
				opts->raw_addr = optarg;
				char *comma = strchr(optarg, ',');
				if (comma == NULL) {
					perr(TAG_ARGSPARSE, "Invalid address '%s' (must be in the form address,port)", optarg);
					return EXIT_FAILURE;
				}
				*comma = 0;
				opts->raw_port = comma + 1;
				break;
			case '?':
				if (strchr("dtsca", optopt))
					perr(TAG_ARGSPARSE, "Option -%c requires an argument.", optopt);
				else if (isprint(optopt))
					perr(TAG_ARGSPARSE, "Unknown option -%c.", optopt);
				else
					perr(TAG_ARGSPARSE, "Unknown option character \\x%x.", optopt);
				return EXIT_FAILURE;
			default:
				break;
		}
	}

	// post-parse checks
	if (opts->operation == 0) {
		perr(TAG_ARGSPARSE, "One of -W or -R is required");
		return EXIT_FAILURE;
	}

	if (opts->filename == NULL || opts->filename_len == 0) {
		perr(TAG_ARGSPARSE, "A valid file name is required");
		return EXIT_FAILURE;
	}

	// test if the file to be sent exists
	if (opts->operation == OP_WRQ) {
		struct stat buffer;
		if (stat(opts->filename, &buffer) == -1) {
			perr(TAG_ARGSPARSE, "Can't send a non-existent file");
			return EXIT_FAILURE;
		}
	}

	// TODO: limit block_size by the lowest MTU of all the interfaces

	return EXIT_SUCCESS;
}
