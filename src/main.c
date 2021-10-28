#include <stdlib.h>
#include <string.h>

#include "client.h"

int main(int argc, char *argv[]) {
	options_t opts;
	if (parse_options(argc, argv, &opts) != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	client_t client = client_init(&opts);
	if (client == NULL) {
		return EXIT_FAILURE;
	}

	if (client_run(client) != EXIT_SUCCESS) {
		client_free(&client);
		return EXIT_FAILURE;
	}

	client_free(&client);
	return EXIT_SUCCESS;
}
