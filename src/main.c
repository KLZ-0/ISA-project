#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "util.h"

#define PTR_BUFFER_SIZE 1024

char buffer[BUFSIZ] = {0};
char *argv[PTR_BUFFER_SIZE];

int main() {
	options_t opts;

	while (fgets(buffer, BUFSIZ, stdin)) {
		int argc = make_argv(buffer, argv, PTR_BUFFER_SIZE);
		if (argc == ARGC_ERROR) {
			fprintf(stderr, "Input error\n");
			continue;
		}

		if (parse_options(argc, argv, &opts) != EXIT_SUCCESS) {
			continue;
		}
	}


	//
	//	client_t client = client_init(&opts);
	//	if (client == NULL) {
	//		return EXIT_FAILURE;
	//	}
	//
	//	if (client_run(client) != EXIT_SUCCESS) {
	//		client_free(&client);
	//		return EXIT_FAILURE;
	//	}
	//
	//	client_free(&client);
	//	return EXIT_SUCCESS;
}
