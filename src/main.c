#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "util.h"

#define PTR_BUFFER_SIZE 1024

char buffer[BUFSIZ] = {0};
char *ptr_buffer[PTR_BUFFER_SIZE];

int main() {
	while (fgets(buffer, BUFSIZ, stdin)) {
		int c = make_argv(buffer, ptr_buffer, PTR_BUFFER_SIZE);

		printf("%d\n", c);
		for (int i = 0; i < c; i++) {
			printf("%s\n", ptr_buffer[i]);
		}
	}

	//	options_t opts;
	//	if (parse_options(argc, argv, &opts) != EXIT_SUCCESS) {
	//		return EXIT_FAILURE;
	//	}
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
