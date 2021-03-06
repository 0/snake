#include <stdlib.h>
#include <sys/time.h>

#include "game.h"
#include "interface.h"

const char *string_COD(enum cod cause) {
	switch (cause) {
		case DEATH_SELF:
			return "hit yourself";
		case DEATH_REVERSE:
			return "reversed into yourself";
		case DEATH_PORTAL:
			return "crashed into a portal";
		case DEATH_QUIT:
			return "quit";
		case DEATH_WALL:
			return "plowed into a wall";
		default:
			return "died mysteriously";
	}
}

void place_food(int cols, int lines, struct snake s, struct map m, struct posn *food, struct posn *portal) {
	do {
		food->x = rand() % cols;
		food->y = rand() % lines;
	} while (collide_with_snake(s.head, *food) || collide_with_wall(m, *food, CH_WALL) || in_snake_path(s, *food));

	do {
		portal->x = rand() % cols;
		portal->y = rand() % lines;
	} while (collide_with_snake(s.head, *portal) || collide_with_wall(m, *portal, CH_WALL) || in_snake_path(s, *portal) || food->x == portal->x || food->y == portal->y);
}

enum move_snake_result move_snake(int cols, int lines, struct snake *s, struct map m, struct posn *food, struct posn *portal, unsigned int length_max) {
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

	if (collide_with_snake(s->head->next, s->head->p))
		return MOVED_SNAKE_SELF;

	if (s->head->p.x == portal->x && s->head->p.y == portal->y)
		return MOVED_SNAKE_PORTAL;

	if (s->head->p.x == food->x && s->head->p.y == food->y) {
		extend_snake(s, rand() % (GROWTH_MAX - GROWTH_MIN) + GROWTH_MIN, length_max);
		s->head->p.x = portal->x;
		s->head->p.y = portal->y;

		place_food(cols, lines, *s, m, food, portal);

		return MOVED_SNAKE_FOOD;
	}

	if (collide_with_wall(m, s->head->p, CH_WALL))
		return MOVED_SNAKE_WALL;

	return MOVED_SNAKE_BORING;
}

unsigned int dfps_to_delay(unsigned int dfps) {
	if (dfps > 0)
		return 10 * 2 * (1000 * 1000) / (dfps * (H_COEF + V_COEF));
	else
		return -1;
}

unsigned int speed_up(unsigned int dfps, unsigned int dfps_max, unsigned int dfps_diff) {
	return (dfps_diff > dfps_max || dfps > dfps_max - dfps_diff) ? dfps_max : dfps + dfps_diff;
}
