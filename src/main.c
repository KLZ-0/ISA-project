#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "connection.h"
#include "util.h"

#define PTR_BUFFER_SIZE 1024

char buffer[BUFSIZ] = {0};
char *argv[PTR_BUFFER_SIZE];

int main() {
	options_t opts;
	client_t client = client_init(&opts);

	while (printf("> "), fflush(stdout), fgets(buffer, BUFSIZ, stdin)) {
		int argc = make_argv(buffer, argv, PTR_BUFFER_SIZE);
		if (argc == ARGC_ERROR) {
			perr(TAG_INPUT, "argv conversion failed");
			continue;
		}

		if (parse_options(argc, argv, &opts) != EXIT_SUCCESS) {
			continue;
		}

		client_conn_init(client);

		client_run(client);

		client_conn_close(client);
	}

	fputc('\n', stdout);
	client_free(&client);
	return EXIT_SUCCESS;

	//	if (client == NULL) {
	//		return EXIT_FAILURE;
	//	}
	//
	//	if (client_run(client) != EXIT_SUCCESS) {
	//		client_free(&client);
	//		return EXIT_FAILURE;
	//	}
}
