#include <stddef.h>

#include "snake.h"
#include "things.h"

int collideWithSnake(struct block *head, int x, int y) {
	while (head) {
		if (head->p.x == x && head->p.y == y)
			return 1;

		head = head->next;
	}

	return 0;
}

int inSnakePath(struct snake s, int x, int y) {
	if (isSnakeVertical(s)) {
		return (x == s.head->p.x);
	} else {
		return (y == s.head->p.y);
	}
}

unsigned int extendSnake(struct snake *s, unsigned int length, unsigned int length_max) {
	unsigned int max_increase = length_max - s->len;
	unsigned int increase = length < max_increase ? length : max_increase;

	unsigned int i;

	for (i = 0; i < increase; ++i) {
		struct block *new = fetchBlock();

		new->p.x = s->tail->p.x;
		new->p.y = s->tail->p.y;
		new->prev = s->tail;
		new->next = NULL;

		s->tail->next = new;
		s->tail = new;
	}

	s->len += increase;

	return length - increase;
}

void reverseSnake(struct snake *s) {
	struct block *b = s->head;
	struct block *b_tmp;

	while (b) {
		b_tmp = b->next;
		b->next = b->prev;
		b->prev = b_tmp;

		b = b_tmp;
	}

	b_tmp = s->head;
	s->head = s->tail;
	s->tail = b_tmp;
}

int isSnakeVertical(struct snake s) {
	return (s.dir == NORTH || s.dir == SOUTH);
}
