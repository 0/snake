#include <stdlib.h>
#include <sys/time.h>

#include "game.h"

const char *stringCOD(enum cod cause) {
	switch (cause) {
		case DEATH_SELF:
			return "hit yourself";
		case DEATH_REVERSE:
			return "reversed into yourself";
		case DEATH_PORTAL:
			return "crashed into a portal";
		case DEATH_QUIT:
			return "quit";
		default:
			return "died mysteriously";
	}
}

void placeFood(int cols, int lines, struct snake s, struct posn *food, struct posn *portal) {
	do {
		food->x = rand() % cols;
		food->y = rand() % lines;
	} while (0 != collideWithSnake(s.head, food->x, food->y));

	do {
		portal->x = rand() % cols;
		portal->y = rand() % lines;
	} while (0 != collideWithSnake(s.head, portal->x, portal->y) || food->x == portal->x || food->y == portal->y);
}

int moveSnake(int cols, int lines, struct snake *s, struct posn *food, struct posn *portal, unsigned int length_max) {
	struct block *b;

	b = s->tail;
	while (b->prev) {
		b->p.x = b->prev->p.x;
		b->p.y = b->prev->p.y;
		b = b->prev;
	}

	switch (s->dir) {
		case NORTH:
			s->head->p.y--;
			if (s->head->p.y < 0)
				s->head->p.y = lines - 1;
			break;
		case EAST:
			s->head->p.x++;
			if (s->head->p.x >= cols)
				s->head->p.x = 0;
			break;
		case SOUTH:
			s->head->p.y++;
			if (s->head->p.y >= lines)
				s->head->p.y = 0;
			break;
		case WEST:
			s->head->p.x--;
			if (s->head->p.x < 0)
				s->head->p.x = cols - 1;
			break;
		default:
			break;
	}

	if (1 == collideWithSnake(s->head->next, s->head->p.x, s->head->p.y))
		return 1;

	if (s->head->p.x == portal->x && s->head->p.y == portal->y)
		return 2;

	if (s->head->p.x == food->x && s->head->p.y == food->y) {
		extendSnake(s, rand() % (GROWTH_MAX - GROWTH_MIN) + GROWTH_MIN, length_max);
		s->head->p.x = portal->x;
		s->head->p.y = portal->y;

		placeFood(cols, lines, *s, food, portal);

		return 3;
	}

	return 0;
}

unsigned int speedUp(unsigned int frame_wait, unsigned int frame_min) {
	if (frame_wait >= FRAME_DIFF) {
		frame_wait -= FRAME_DIFF;
		if (frame_wait < frame_min)
			frame_wait = frame_min;
	}

	return frame_wait;
}
