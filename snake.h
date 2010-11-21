#ifndef __SNAKE_H
#define __SNAKE_H

#include "things.h"

enum direction {
	DEAD,
	NORTH,
	EAST,
	SOUTH,
	WEST
};

struct snake {
	struct block *head;
	struct block *tail;
	unsigned int len;
	enum direction dir;
};

int collideWithSnake(struct block *head, int x, int y);
unsigned int extendSnake(struct snake *s, unsigned int length, unsigned int length_max);
void reverseSnake(struct snake *s);
int isSnakeVertical(struct snake s);

#endif /* __SNAKE_H */
