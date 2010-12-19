#ifndef __THINGS_H
#define __THINGS_H

struct posn {
	int x;
	int y;
};

struct block {
	struct posn p;
	struct block *next;
	struct block *prev;
};

struct map {
	unsigned int width;
	unsigned int height;
	char **tiles;
};

struct block *fetch_block();
int collide_with_wall(struct map m, struct posn p, char wall);

#endif /* __THINGS_H */
