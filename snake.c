#include <stddef.h>

#include "snake.h"
#include "things.h"

int is_dir_opposite(enum direction a, enum direction b) {
	return a != b && (a - NORTH) % 2 == (b - NORTH) % 2;
}

int collide_with_snake(struct block *head, struct posn p) {
	while (head) {
		if (head->p.x == p.x && head->p.y == p.y)
			return 1;

		head = head->next;
	}

	return 0;
}

int in_snake_path(struct snake s, struct posn p) {
	if (is_snake_vertical(s)) {
		return (p.x == s.head->p.x);
	} else {
		return (p.y == s.head->p.y);
	}
}

unsigned int extend_snake(struct snake *s, unsigned int length, unsigned int length_max) {
	unsigned int max_increase = length_max - s->len;
	unsigned int increase = length < max_increase ? length : max_increase;

	unsigned int i;

	for (i = 0; i < increase; ++i) {
		struct block *new = fetch_block();

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

void reverse_snake(struct snake *s) {
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

int is_snake_vertical(struct snake s) {
	return (s.dir == NORTH || s.dir == SOUTH);
}
