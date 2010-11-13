#include <curses.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define CH_HEAD   '@'
#define CH_BODY   '#'
#define CH_FOOD   '*'
#define CH_PORTAL '^'
#define CH_VOID   ' '

#define KEY_UP_ALT    'w'
#define KEY_RIGHT_ALT 'd'
#define KEY_DOWN_ALT  's'
#define KEY_LEFT_ALT  'a'
#define KEY_PAUSE     'p'
#define KEY_QUIT      'q'

#define GROWTH_MIN 10
#define GROWTH_MAX 100

#define LENGTH_MAX (10 * 1000)

#define FPS_MIN       1
#define FPS_MAX       1000
#define FPS_MAX_CHARS 4
#define FPS_INIT      80
#define FRAME_DIFF    5

#define SLEEP_MAX (10 * 1000)

#define H_COEF 2
#define V_COEF 3

enum direction {
	DEAD,
	NORTH,
	EAST,
	SOUTH,
	WEST
};

enum item_color {
	COLOR_LEAD = 1,
	COLOR_BLOCK,
	COLOR_FOOD,
	COLOR_PORTAL
};

enum cod {
	DEATH_UNKNOWN,
	DEATH_SELF,
	DEATH_REVERSE,
	DEATH_PORTAL,
	DEATH_QUIT
};

struct posn {
	int x;
	int y;
};

struct block {
	struct posn p;
	struct block *next;
	struct block *prev;
};

struct snake {
	struct block *head;
	struct block *tail;
	unsigned int len;
};

int use_color = 0;

volatile int bailing = 0;

unsigned int delay_to_fps(unsigned int delay) {
	if (delay > 0)
		return 2 * (1000 * 1000) / (delay * (H_COEF + V_COEF));
	else
		return -1;
}

unsigned int fps_to_delay(unsigned int fps) {
	if (fps > 0)
		return 2 * (1000 * 1000) / (fps * (H_COEF + V_COEF));
	else
		return -1;
}

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

void cleanup(struct snake s, time_t time_start, time_t time_stop, unsigned int frame_wait,
		enum cod cause_of_death, int proper_exit) {
	double time_total;
	struct block *b;

	if (!proper_exit)
		time_stop = time(NULL);

	time_total = difftime(time_stop, time_start);

	b = s.head;
	while (b) {
		struct block *b_tmp = b->next;
		free(b);
		b = b_tmp;
	}

	endwin();

	if (time_total <= 0 || LINES <= 0 || COLS <= 0) {
		printf("You didn't even play!\n");
	} else {
		const char *cause = stringCOD(cause_of_death);
		printf("%s with %d segments in %.0f seconds on %d lines and %d columns at %d frames per second\n",
			cause, s.len, time_total, LINES, COLS, delay_to_fps(frame_wait));
	}
}

void bail(int sig) {
	bailing = sig;
}

void do_color(enum item_color c, int on) {
	if (use_color) {
		if (on) {
			attron(COLOR_PAIR(c));
		} else {
			attroff(COLOR_PAIR(c));
		}
	}
}

int collideWithSnake(struct block *head, int x, int y) {
	while (head) {
		if (head->p.x == x && head->p.y == y)
			return 1;

		head = head->next;
	}

	return 0;
}

void placeFood(struct snake s, struct posn *food, struct posn *portal) {
	if (food->x >= 0) {
		mvaddch(food->y, food->x, CH_VOID);
		mvaddch(portal->y, portal->x, CH_VOID);
	}

	do {
		food->x = rand() % COLS;
		food->y = rand() % LINES;
	} while (0 != collideWithSnake(s.head, food->x, food->y));

	do_color(COLOR_FOOD, 1);
	mvaddch(food->y, food->x, CH_FOOD);
	do_color(COLOR_FOOD, 0);

	do {
		portal->x = rand() % COLS;
		portal->y = rand() % LINES;
	} while (0 != collideWithSnake(s.head, portal->x, portal->y) || food->x == portal->x || food->y == portal->y);

	do_color(COLOR_PORTAL, 1);
	mvaddch(portal->y, portal->x, CH_PORTAL);
	do_color(COLOR_PORTAL, 0);
}

struct block *fetchBlock() {
	struct block *ret = calloc(1, sizeof(struct block));

	if (NULL == ret) {
		fprintf(stderr, "Can't get memory for the snake!\n");
		exit(1);
	}

	return ret;
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

int moveSnake(struct snake *s, enum direction dir, struct posn *food, struct posn *portal,
		unsigned int length_max) {
	struct block *b;

	mvaddch(s->tail->p.y, s->tail->p.x, CH_VOID);

	b = s->tail;
	while (b->prev) {
		b->p.x = b->prev->p.x;
		b->p.y = b->prev->p.y;
		b = b->prev;
	}

	switch (dir) {
		case NORTH:
			s->head->p.y--;
			if (s->head->p.y < 0)
				s->head->p.y = LINES - 1;
			break;
		case EAST:
			s->head->p.x++;
			if (s->head->p.x >= COLS)
				s->head->p.x = 0;
			break;
		case SOUTH:
			s->head->p.y++;
			if (s->head->p.y >= LINES)
				s->head->p.y = 0;
			break;
		case WEST:
			s->head->p.x--;
			if (s->head->p.x < 0)
				s->head->p.x = COLS - 1;
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
		placeFood(*s, food, portal);
	}

	do_color(COLOR_LEAD, 1);
	mvaddch(s->head->p.y, s->head->p.x, CH_HEAD);
	do_color(COLOR_LEAD, 0);

	if (s->head->next) {
		do_color(COLOR_BLOCK, 1);
		mvaddch(s->head->next->p.y, s->head->next->p.x, CH_BODY);
		do_color(COLOR_BLOCK, 0);
	}

	return 0;
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

unsigned int speedUp(unsigned int frame_wait, unsigned int frame_min) {
	if (frame_wait >= FRAME_DIFF) {
		frame_wait -= FRAME_DIFF;
		if (frame_wait < frame_min)
			frame_wait = frame_min;
	}

	return frame_wait;
}

void redraw(struct block *head, struct posn food, struct posn portal) {
	struct block *b;

	erase();

	do_color(COLOR_LEAD, 1);
	mvaddch(head->p.y, head->p.x, CH_HEAD);
	do_color(COLOR_LEAD, 0);

	b = head->next;
	do_color(COLOR_BLOCK, 1);
	while (b) {
		mvaddch(b->p.y, b->p.x, CH_BODY);
		b = b->next;
	}
	do_color(COLOR_BLOCK, 0);

	do_color(COLOR_FOOD, 1);
	mvaddch(food.y, food.x, CH_FOOD);
	do_color(COLOR_FOOD, 0);

	do_color(COLOR_PORTAL, 1);
	mvaddch(portal.y, portal.x, CH_PORTAL);
	do_color(COLOR_PORTAL, 0);
}

const char *generate_header() {
	switch (rand() % 10) {
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

void show_usage(char *cmd) {
	printf(
"%s\n"
"\n"
"USAGE: %s [options]\n"
"\n"
"OPTIONS:\n",
		generate_header(), cmd);

	printf(
"  Display\n"
"    --bright\n"
"        Brighter object coloring. Default off.\n"
"    --color\n"
"        Enable color. Default on.\n"
"    --instructions\n"
"        Show instructions. Default off.\n"
"    --fps-display\n"
"        Show current framerate. Default off.\n");

	printf(
"  Gameplay\n"
"    --fps-init <num>\n"
"        Initial framerate (integer). Default: %d\n"
"    --fps-max <num>\n"
"        Maximum framerate (integer). Default: %d\n"
"    --length-init <num>\n"
"        Initial snake length (integer). Default: 0\n"
"    --length-max <num>\n"
"        Maximum snake length (integer). Default: %d\n",
		FPS_INIT, FPS_MAX, LENGTH_MAX);

	printf(
"  Miscellaneous\n"
"    --help\n"
"        Print this and exit.\n"
"    --no-<foo>\n"
"        Disable boolean option <foo>.\n");

	printf(
"\n"
"Framerates must be between %d and %d, inclusive.\n"
"Lengths must be between 0 and %d, inclusive.\n",
		FPS_MIN, FPS_MAX, LENGTH_MAX);
}

int main(int argc, char **argv) {
	static int color_flag = 1;
	static int bright_flag = 0;
	static int instructions_flag = 0;
	static int fps_display_flag = 0;

	int fps_init = FPS_INIT;
	int fps_max = FPS_MAX;
	unsigned int length_init = 0;

	struct option longopts[] = {
		{"bright", no_argument, &bright_flag, 1},
		{"no-bright", no_argument, &bright_flag, 0},
		{"color", no_argument, &color_flag, 1},
		{"no-color", no_argument, &color_flag, 0},
		{"instructions", no_argument, &instructions_flag, 1},
		{"no-instructions", no_argument, &instructions_flag, 0},
		{"fps-display", no_argument, &fps_display_flag, 1},
		{"no-fps-display", no_argument, &fps_display_flag, 0},
		{"fps-init", required_argument, 0, 'i'},
		{"fps-max", required_argument, 0, 'm'},
		{"length-init", required_argument, 0, 'j'},
		{"length-max", required_argument, 0, 'n'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	int opterr_flag = 0;
	int c = 0;

	char fps_display_buf[FPS_MAX_CHARS + 1];

	int cursor_visible = 0;

	struct snake s = {NULL, NULL, 0};

	struct posn food = {-1, -1};
	struct posn portal;

	unsigned int frame_wait;
	unsigned int frame_min;

	enum cod cause_of_death = DEATH_UNKNOWN;

	enum direction dir;
	int paused = 0;

	time_t time_start, time_stop;

	unsigned int length_max = LENGTH_MAX;

	struct timeval next_frame;
	gettimeofday(&next_frame, NULL);

	srand(time(NULL));

	while (c != -1) {
		char *p;
		c = getopt_long(argc, argv, "", longopts, NULL);

		switch (c) {
			case 'h':
				show_usage(argv[0]);
				return 0;
			case 'i':
				fps_init = strtol(optarg, &p, 10);
				if (errno || *p || fps_init < FPS_MIN || fps_init > FPS_MAX) {
					fprintf(stderr, "Invalid value for fps-init: %d (%s)\n", fps_init, optarg);
					opterr_flag = 1;
				}
				break;
			case 'm':
				fps_max = strtol(optarg, &p, 10);
				if (errno || *p || fps_max < FPS_MIN || fps_max > FPS_MAX) {
					fprintf(stderr, "Invalid value for fps-max: %d (%s)\n", fps_max, optarg);
					opterr_flag = 1;
				}
				break;
			case 'j':
				length_init = strtol(optarg, &p, 10);
				if (errno || *p || length_init > LENGTH_MAX) {
					fprintf(stderr, "Invalid value for length-init: %u (%s)\n", length_init, optarg);
					opterr_flag = 1;
				}
				break;
			case 'n':
				length_max = strtol(optarg, &p, 10);
				if (errno || *p || length_max > LENGTH_MAX) {
					fprintf(stderr, "Invalid value for length-max: %u (%s)\n", length_max, optarg);
					opterr_flag = 1;
				}
				break;
			case '?':
				opterr_flag = 1;
				break;
		}
	}

	if (fps_init > fps_max) {
		fprintf(stderr, "Initial framerate cannot be higher than the maximum.\n");
		opterr_flag = 1;
	}

	if (length_init > length_max) {
		fprintf(stderr, "Initial length cannot be higher than the maximum.\n");
		opterr_flag = 1;
	}

	if (opterr_flag) {
		fprintf(stderr, "Use --help for usage information.\n");
		return 1;
	}

	signal(SIGINT, bail);
	signal(SIGQUIT, bail);

	initscr();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	if (ERR == curs_set(0))
		cursor_visible = 1;
	noecho();

	if (color_flag && has_colors()) {
		use_color = 1;
		start_color();
		init_pair(COLOR_LEAD, COLOR_BLUE, bright_flag ? COLOR_BLUE : COLOR_BLACK);
		init_pair(COLOR_BLOCK, COLOR_CYAN, bright_flag ? COLOR_CYAN : COLOR_BLACK);
		init_pair(COLOR_FOOD, COLOR_GREEN, bright_flag ? COLOR_GREEN : COLOR_BLACK);
		init_pair(COLOR_PORTAL, COLOR_RED, bright_flag ? COLOR_RED : COLOR_BLACK);
	}

	if (instructions_flag) {
		mvaddstr(0, 0, "move:  arrows or WASD");
		mvaddstr(1, 0, "stop:  Q or die");
		mvaddstr(2, 0, "pause: P");
		mvaddstr(3, 0, "quit:  any key");
	}

	attron(A_BOLD);

	frame_wait = fps_to_delay(fps_init);
	frame_min = fps_to_delay(fps_max);

	s.head = s.tail = fetchBlock();

	s.head->p.x = COLS / 2;
	s.head->p.y = LINES / 2;

	extendSnake(&s, length_init, length_max);

	placeFood(s, &food, &portal);

	dir = NORTH + rand() % 4;

	time_start = time_stop = time(NULL);

	while (dir != DEAD) {
		struct timeval until_next;
		unsigned int sleep_time;

		int update = 0;
		int rev = 0;

		struct timeval now;
		gettimeofday(&now, NULL);

		if (timercmp(&now, &next_frame, >)) {
			sleep_time = frame_wait * (NORTH == dir || SOUTH == dir ? V_COEF : H_COEF);

			until_next.tv_sec = sleep_time / (1000 * 1000);
			until_next.tv_usec = sleep_time % (1000 * 1000);
			timeradd(&now, &until_next, &next_frame);

			update = 1;
		} else {
			timersub(&next_frame, &now, &until_next);
			sleep_time = until_next.tv_sec * (1000 * 1000) + until_next.tv_usec;
		}

		while (ERR != (c = getch())) {
			switch (c) {
				case KEY_UP:
				case KEY_UP_ALT:
					if (SOUTH == dir) {
						reverseSnake(&s);
						rev = 1;
					}
					dir = NORTH;
					frame_wait = speedUp(frame_wait, frame_min);
					break;
				case KEY_RIGHT:
				case KEY_RIGHT_ALT:
					if (WEST == dir) {
						reverseSnake(&s);
						rev = 1;
					}
					dir = EAST;
					frame_wait = speedUp(frame_wait, frame_min);
					break;
				case KEY_DOWN:
				case KEY_DOWN_ALT:
					if (NORTH == dir) {
						reverseSnake(&s);
						rev = 1;
					}
					dir = SOUTH;
					frame_wait = speedUp(frame_wait, frame_min);
					break;
				case KEY_LEFT:
				case KEY_LEFT_ALT:
					if (EAST == dir) {
						reverseSnake(&s);
						rev = 1;
					}
					dir = WEST;
					frame_wait = speedUp(frame_wait, frame_min);
					break;
				case KEY_PAUSE:
					paused = !paused;
					if (paused)
						mvaddstr(s.head->p.y, s.head->p.x, "PAUSED");
					else
						redraw(s.head, food, portal);

					break;
				case KEY_QUIT:
					cause_of_death = DEATH_QUIT;

					cleanup(s, time_start, time_stop, frame_wait, cause_of_death, 0);
					return 0;
				case KEY_RESIZE:
					if (s.head->p.x > COLS)
						s.head->p.x = COLS - 1;

					if (s.head->p.y > LINES)
						s.head->p.y = LINES - 1;

					placeFood(s, &food, &portal);
					break;
				default:
					break;
			}
		}

		if (!paused && update) {
			switch (moveSnake(&s, dir, &food, &portal, length_max)) {
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
		}

		if (fps_display_flag) {
			sprintf(fps_display_buf, "%d", delay_to_fps(frame_wait));
			mvaddstr(LINES - 1, 0, fps_display_buf);
		}

		if (bailing) {
			cleanup(s, time_start, time_stop, frame_wait, cause_of_death, 0);
			return 1;
		}

		if (cursor_visible)
			move(0, 0);

		usleep(sleep_time > SLEEP_MAX ? SLEEP_MAX : sleep_time);
	}

	time_stop = time(NULL);
	mvaddstr(s.head->p.y, s.head->p.x, "DEAD!");

	nodelay(stdscr, FALSE);
	getch();

	cleanup(s, time_start, time_stop, frame_wait, cause_of_death, 1);
	return 0;
}
