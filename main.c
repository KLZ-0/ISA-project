#include <stdio.h>
#include <stdlib.h>

#include "client.h"

int main() {
//	client_t client = client_init("localhost");
//	client_t client = client_init("127.0.0.1");
	client_t client = client_init("::1");
	if (client == NULL) {
		return EXIT_FAILURE;
	}

	client_free(&client);
	return EXIT_SUCCESS;
}
