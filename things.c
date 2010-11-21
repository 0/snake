#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "things.h"

struct block *fetchBlock() {
	struct block *ret = calloc(1, sizeof(struct block));

	if (NULL == ret) {
		fprintf(stderr, "Can't get memory for the snake!\n");
		exit(1);
	}

	return ret;
}
