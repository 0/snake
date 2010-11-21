#ifndef __GAME_H
#define __GAME_H

#include "snake.h"
#include "things.h"

#define GROWTH_MIN 10
#define GROWTH_MAX 100

#define LENGTH_MAX (10 * 1000)

#define FPS_MIN       1
#define FPS_MAX       1000
#define FPS_MAX_CHARS 4
#define FPS_INIT      80

#define SLEEP_MAX (10 * 1000)

#define FRAME_DIFF    5

enum cod {
	DEATH_UNKNOWN,
	DEATH_SELF,
	DEATH_REVERSE,
	DEATH_PORTAL,
	DEATH_QUIT
};

const char *stringCOD(enum cod cause);
void placeFood(int cols, int lines, struct snake s, struct posn *food, struct posn *portal);
int moveSnake(int cols, int lines, struct snake *s, struct posn *food, struct posn *portal, unsigned int length_max);
unsigned int speedUp(unsigned int frame_wait, unsigned int frame_min);

#endif /* __GAME_H */
