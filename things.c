#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "things.h"

struct block *fetch_block() {
	struct block *ret = calloc(1, sizeof(struct block));

	if (NULL == ret) {
		fprintf(stderr, "Can't get memory for the snake!\n");
		exit(1);
	}

	return ret;
}

int collide_with_wall(struct map m, struct posn p, char wall) {
	if (p.x < 0 || (unsigned int)p.x >= m.width || p.y < 0 || (unsigned int)p.y >= m.height)
		return 0;

	return wall == m.tiles[p.y][p.x];
}
