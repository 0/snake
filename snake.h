#ifndef __SNAKE_H
#define __SNAKE_H

#include "things.h"

enum direction {
	NO_DIR,
	DEAD,
	NORTH,
	EAST,
	SOUTH,
	WEST,
};

struct snake {
	struct block *head;
	struct block *tail;
	unsigned int len;
	enum direction dir;
};

int is_dir_opposite(enum direction a, enum direction b);
int collide_with_snake(struct block *head, struct posn p);
int in_snake_path(struct snake s, struct posn p);
unsigned int extend_snake(struct snake *s, unsigned int length, unsigned int length_max);
void reverse_snake(struct snake *s);
int is_snake_vertical(struct snake s);

#endif /* __SNAKE_H */
