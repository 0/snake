#ifndef __GAME_H
#define __GAME_H

#include "snake.h"
#include "things.h"

#define GROWTH_MIN 10
#define GROWTH_MAX 100

#define LENGTH_MAX (10 * 1000)

#define DFPS_MIN  10
#define DFPS_MAX  10000
#define DFPS_INIT 800

#define DFPS_MAX_CHARS 5

#define ACCEL_MIN     0
#define ACCEL_DEFAULT 5
#define ACCEL_MAX     500

#define SLEEP_MAX (10 * 1000)

enum cod {
	DEATH_UNKNOWN,
	DEATH_SELF,
	DEATH_REVERSE,
	DEATH_PORTAL,
	DEATH_QUIT,
};

const char *string_COD(enum cod cause);
void place_food(int cols, int lines, struct snake s, struct posn *food, struct posn *portal);
int move_snake(int cols, int lines, struct snake *s, struct posn *food, struct posn *portal, unsigned int length_max);
unsigned int dfps_to_delay(unsigned int dfps);
unsigned int speed_up(unsigned int dfps, unsigned int dfps_max, unsigned int dfps_diff);

#endif /* __GAME_H */
