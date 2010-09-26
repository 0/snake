#include <curses.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#define LEAD	'@'
#define BLOCK	'#'
#define FOOD	'*'
#define PORTAL	'^'
#define VOID	' '

#define HEAD	snake [0]
#define TAIL	snake [len]

#define KEY_UP_ALT		'w'
#define KEY_RIGHT_ALT	'd'
#define KEY_DOWN_ALT	's'
#define KEY_LEFT_ALT	'a'
#define KEY_QUIT		'q'

#define START_LEN	0
#define GROWTH_MIN	10
#define GROWTH_MAX	100
#define MAX_LEN		10000

#define FPS_MIN		1
#define FPS_MAX		1000
#define FPS_INIT	80
#define FRAME_DIFF	5

#define H_COEF		2
#define V_COEF		3


typedef enum {
	DEAD,
	NORTH,
	EAST,
	SOUTH,
	WEST
} direction_t;

typedef enum {
	COLOR_LEAD = 1,
	COLOR_BLOCK,
	COLOR_FOOD,
	COLOR_PORTAL
} item_color_t;

typedef enum {
	DEATH_UNKNOWN,
	DEATH_SELF,
	DEATH_REVERSE,
	DEATH_PORTAL,
	DEATH_QUIT
} cod_t;


typedef struct {
	int x;
	int y;
} block_t;


time_t time_start;
unsigned int frame_wait;

direction_t dir;
unsigned int len;
int rev;

block_t *snake;
block_t food;
block_t portal;

cod_t cause_of_death = DEATH_UNKNOWN;

int use_color = 0;

unsigned int frame_min;


static void finish (int sig);


void do_color (item_color_t c, int on) {
	if (use_color) {
		if (on) {
			attron (COLOR_PAIR (c));
		} else {
			attroff (COLOR_PAIR (c));
		}
	}
}

int collideWithSnake (int x, int y) {
	unsigned int i;
	for (i = 1; i <= len; ++i) {
		if (snake [i].x == x && snake [i].y == y)
			return 1;
	}
	return 0;
}

void placeFood () {
	int done = 0;
	while (!done) {
		food.x = rand () % COLS;
		food.y = rand () % LINES;
		if (0 == collideWithSnake (food.x, food.y) && food.x != portal.x && portal.y != food.y)
			done = 1;
	}
	do_color (COLOR_FOOD, 1);
	mvaddch (food.y, food.x, FOOD);
	do_color (COLOR_FOOD, 0);

	done = 0;
	while (!done) {
		portal.x = rand () % COLS;
		portal.y = rand () % LINES;
		if (0 == collideWithSnake (portal.x, portal.y) && food.x != portal.x && food.y != portal.y)
			done = 1;
	}
	do_color (COLOR_PORTAL, 1);
	mvaddch (portal.y, portal.x, PORTAL);
	do_color (COLOR_PORTAL, 0);
}

block_t* fetchSnake () {
	block_t *tmp = malloc ((1 + MAX_LEN) * sizeof (block_t));
	if (tmp == NULL) {
		fprintf (stderr, "Can't malloc for the snake!\n");
		finish (0);
	}
	return tmp;
}

unsigned int extendSnake (unsigned int length) {
	unsigned int max_increase = MAX_LEN - len;
	unsigned int increase = length < max_increase ? length : max_increase;

	unsigned int i;

	for (i = 1; i <= increase; ++i) {
		snake [len + i].x = TAIL.x;
		snake [len + i].y = TAIL.y;
	}

	len += increase;

	return length - increase;
}

int moveSnake () {
	int i;

	mvaddch (TAIL.y, TAIL.x, VOID);

	for (i = len; i > 0; --i) {
		snake [i].x = snake [i - 1].x;
		snake [i].y = snake [i - 1].y;
	}	
	switch (dir) {
		case NORTH:
			HEAD.y--;
			if (HEAD.y < 0)
				HEAD.y = LINES - 1;
			break;
		case EAST:
			HEAD.x++;
			if (HEAD.x >= COLS)
				HEAD.x = 0;
			break;
		case SOUTH:
			HEAD.y++;
			if (HEAD.y >= LINES)
				HEAD.y = 0;
			break;
		case WEST:
			HEAD.x--;
			if (HEAD.x < 0)
				HEAD.x = COLS - 1;
			break;
		default:
			break;
	}
	if (1 == collideWithSnake (HEAD.x, HEAD.y))
		return 1;
	if (HEAD.x == portal.x && HEAD.y == portal.y)
		return 2;
	if (HEAD.x == food.x && HEAD.y == food.y) {
		extendSnake (rand () % (GROWTH_MAX - GROWTH_MIN) + GROWTH_MIN);
		HEAD.x = portal.x;
		HEAD.y = portal.y;
		mvaddch (food.y, food.x, VOID);
		placeFood ();
	}

	do_color (COLOR_LEAD, 1);
	mvaddch (HEAD.y, HEAD.x, LEAD);
	do_color (COLOR_LEAD, 0);

	if (len > 0) {
		do_color (COLOR_BLOCK, 1);
		mvaddch (snake [1].y, snake [1].x, BLOCK);
		do_color (COLOR_BLOCK, 0);
	}

	return 0;
}

block_t* reverseSnake () {
	unsigned int i;
	block_t *tmp = fetchSnake ();
	for (i = 0; i <= len; ++i) {
		tmp [i].x = snake [len - i].x;
		tmp [i].y = snake [len - i].y;
	}
	free (snake);

	rev = 1;
	return tmp;
}

void speedUp () {
	if (frame_wait >= FRAME_DIFF) {
		frame_wait -= FRAME_DIFF;
		if (frame_wait < frame_min)
			frame_wait = frame_min;
	}
}

unsigned int delay_to_fps (unsigned int delay) {
	if (delay > 0)
		return 2 * 1000000 / (delay * (H_COEF + V_COEF));
	else
		return -1;
}

unsigned int fps_to_delay (unsigned int fps) {
	if (fps > 0)
		return 2 * 1000000 / (fps * (H_COEF + V_COEF));
	else
		return -1;
}

const char* stringCOD (cod_t cause) {
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

const char* generate_header () {
	switch (rand () % 10) {
		case 0:
			return "                 _        \n"
				" ___ _ __   __ _| | _____ \n"
				"/ __| '_ \\ / _` | |/ / _ \\\n"
				"\\__ \\ | | | (_| |   <  __/\n"
				"|___/_| |_|\\__,_|_|\\_\\___|";
		case 1:
			return "Yet another snake clone...";
		case 2:
			return "Snake's Not A Recursive Acronym";
		case 3:
			return "Snake's Not A Knotted Eel";
		case 4:
			return "snake!";
		default:
			return "snake";
	}
}

void show_usage (char *cmd) {
	printf (
		"%s\n"
		"\n"
		"Usage: %s [options]\n"
		"\n"
		"Options:\n"
		"\t--bright\n\t\tEnable brighter object coloring. Default off.\n"
        "\t--color\n\t\tEnable color. Default on.\n"
        "\t--instructions\n\t\tDisplay instructions. Default on.\n"
		"\t--fps-init\n\t\tInitial framerate as an integer. Default: %d\n"
		"\t--fps-max\n\t\tMaximum framerate as an integer. Default: %d\n"
		"\t--help\n\t\tPrint this.\n"
		"\n"
		"Boolean flags --foo have corresponding --no-foo.\n"
		"Framerates must be between %d and %d, inclusive.\n",
		generate_header (), cmd, FPS_INIT, FPS_MAX, FPS_MIN, FPS_MAX);
}


int main (int argc, char **argv) {
	static int color_flag = 1;
	static int bright_flag = 0;
	static int instructions_flag = 1;

	int fps_init = FPS_INIT;
	int fps_max = FPS_MAX;

	struct option longopts [] = {
		{"bright", no_argument, &bright_flag, 1},
		{"no-bright", no_argument, &bright_flag, 0},
		{"color", no_argument, &color_flag, 1},
		{"no-color", no_argument, &color_flag, 0},
		{"instructions", no_argument, &instructions_flag, 1},
		{"no-instructions", no_argument, &instructions_flag, 0},
		{"fps-init", required_argument, 0, 'i'},
		{"fps-max", required_argument, 0, 'm'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	int opterr_flag = 0;
	int c = 0;

	srand (time (NULL));

	do {
		char *p;
		c = getopt_long (argc, argv, "", longopts, NULL);

		switch (c) {
			case 'h':
				show_usage (argv[0]);
				exit(0);
			case 'i':
				fps_init = strtol(optarg, &p, 10);
				if (errno || *p || fps_init < FPS_MIN || fps_init > FPS_MAX) {
					fprintf (stderr, "Invalid value for fps-init: %s\n", optarg);
					opterr_flag = 1;
				}
				break;
			case 'm':
				fps_max = strtol(optarg, &p, 10);
				if (errno || *p || fps_max < FPS_MIN || fps_max > FPS_MAX) {
					fprintf (stderr, "Invalid value for fps-max: %s\n", optarg);
					opterr_flag = 1;
				}
				break;
			case '?':
				opterr_flag = 1;
				break;
		}
	} while (c != -1);

	if (fps_init > fps_max) {
		fprintf (stderr, "Initial frame rate cannot be higher than the maximum.\n");
		opterr_flag = 1;
	}

	if (opterr_flag) {
		fprintf (stderr, "Use --help for usage information.\n");
		exit (1);
	}

	signal (SIGINT, finish);

	initscr ();
	keypad (stdscr, TRUE);
	nodelay (stdscr, TRUE);
	curs_set (0);
	noecho ();

	if (color_flag && has_colors ()) {
		use_color = 1;
		start_color();
		init_pair(COLOR_LEAD, COLOR_BLUE, bright_flag ? COLOR_BLUE : COLOR_BLACK);
		init_pair(COLOR_BLOCK, COLOR_CYAN, bright_flag ? COLOR_CYAN : COLOR_BLACK);
		init_pair(COLOR_FOOD, COLOR_GREEN, bright_flag ? COLOR_GREEN : COLOR_BLACK);
		init_pair(COLOR_PORTAL, COLOR_RED, bright_flag ? COLOR_RED : COLOR_BLACK);
	}

	if (instructions_flag) {
		mvaddstr (0, 0, "move: arrows or wasd");
		mvaddstr (1, 0, "stop: q or die");
		mvaddstr (2, 0, "quit: any key");
	}

	attron (A_BOLD);

	len = 0;
	frame_wait = fps_to_delay (fps_init);
	frame_min = fps_to_delay (fps_max);

	snake = fetchSnake ();

	HEAD.x = COLS / 2;
	HEAD.y = LINES / 2;

#if START_LEN > 0
	extendSnake (START_LEN);
#endif

	placeFood ();

	dir = NORTH + rand () % 4;

	time_start = time (NULL);

	while (dir != DEAD) {
		rev = 0;

		switch (getch ()) {
			case KEY_UP:
			case KEY_UP_ALT:
				if (dir == SOUTH)
					snake = reverseSnake ();
				dir = NORTH;
				speedUp ();
				break;
			case KEY_RIGHT:
			case KEY_RIGHT_ALT:
				if (dir == WEST)
					snake = reverseSnake ();
				dir = EAST;
				speedUp ();
				break;
			case KEY_DOWN:
			case KEY_DOWN_ALT:
				if (dir == NORTH)
					snake = reverseSnake ();
				dir = SOUTH;
				speedUp ();
				break;
			case KEY_LEFT:
			case KEY_LEFT_ALT:
				if (dir == EAST)
					snake = reverseSnake ();
				dir = WEST;
				speedUp ();
				break;
			case KEY_QUIT:
				cause_of_death = DEATH_QUIT;
				finish (0);
				break;
			default:
				break;
		}
		switch (moveSnake ()) {
			case 1:
				if (rev)
					cause_of_death = DEATH_REVERSE;
				else
					cause_of_death = DEATH_SELF;
				dir = DEAD;
				break;
			case 2:
				cause_of_death = DEATH_PORTAL;
				dir = DEAD;
				break;
			default:
				break;
		}
		usleep (frame_wait * (NORTH == dir || SOUTH == dir ? V_COEF : H_COEF));
	}

	mvaddstr (HEAD.y, HEAD.x, "DEAD!");

	nodelay (stdscr, FALSE);
	getch ();

	finish (0);

	return 0;
}


static void finish (int sig) {
	double time_total = difftime(time (NULL), time_start);

	free (snake);
	endwin ();

	if (time_total <= 0 || LINES <= 0 || COLS <= 0) {
		printf ("You didn't even play!\n");
	} else {
		const char* cause = stringCOD (cause_of_death);
		printf ("%s with %d segments in %.0f seconds on %d lines and %d columns at %d frames per second\n",
			cause, len, time_total, LINES, COLS, delay_to_fps (frame_wait));
	}

	exit (sig ? 1 : 0);
}
