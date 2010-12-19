#include <curses.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "interface.h"
#include "game.h"
#include "snake.h"
#include "things.h"

int use_color = 0;

volatile int bailing = 0;

void cleanup(struct snake s, time_t time_start, time_t time_stop, unsigned int dfps, enum cod cause_of_death, int proper_exit) {
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
		const char *cause = string_COD(cause_of_death);
		printf("%s with %d segments in %.0f seconds on %d lines and %d columns at %d frames per second\n",
			cause, s.len, time_total, LINES, COLS, dfps / 10);
	}
}

void bail(int sig) {
	bailing = sig;
}

int main(int argc, char **argv) {
	static int color_flag = 1;
	static int bright_flag = 0;
	static int instructions_flag = 0;
	static int dfps_display_flag = 0;

	unsigned int dfps_init = DFPS_INIT;
	unsigned int dfps_max = DFPS_MAX;
	unsigned int acceleration = ACCEL_DEFAULT;

	unsigned int length_init = 0;

	struct option longopts[] = {
		{"acceleration", required_argument, 0, 'a'},
		{"bright", no_argument, &bright_flag, 1},
		{"no-bright", no_argument, &bright_flag, 0},
		{"color", no_argument, &color_flag, 1},
		{"no-color", no_argument, &color_flag, 0},
		{"instructions", no_argument, &instructions_flag, 1},
		{"no-instructions", no_argument, &instructions_flag, 0},
		{"dfps-display", no_argument, &dfps_display_flag, 1},
		{"no-dfps-display", no_argument, &dfps_display_flag, 0},
		{"dfps-init", required_argument, 0, 'i'},
		{"dfps-max", required_argument, 0, 'm'},
		{"length-init", required_argument, 0, 'j'},
		{"length-max", required_argument, 0, 'n'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	int opterr_flag = 0;
	int c = 0;

	char dfps_display_buf[DFPS_MAX_CHARS + 1];

	int cursor_visible = 0;

	struct snake s = {NULL, NULL, 0, DEAD};

	struct posn food = {-1, -1};
	struct posn portal;

	unsigned int dfps_cur;

	enum cod cause_of_death = DEATH_UNKNOWN;

	int paused = 0;

	time_t time_start, time_stop;

	unsigned int length_max = LENGTH_MAX;

	struct timeval next_frame;
	gettimeofday(&next_frame, NULL);

	srand(time(NULL));

	while (c != -1) {
		char *p;
		long n;
		c = getopt_long(argc, argv, "", longopts, NULL);

		switch (c) {
			case 'h':
				show_usage(argv[0]);
				return 0;
			case 'a':
				n = strtol(optarg, &p, 10);
				if (errno || *p || n < ACCEL_MIN || n > ACCEL_MAX) {
					fprintf(stderr, "Invalid value for acceleration: %s\n", optarg);
					opterr_flag = 1;
				} else {
					acceleration = n;
				}
				break;
			case 'i':
				n = strtol(optarg, &p, 10);
				if (errno || *p || n < DFPS_MIN || n > DFPS_MAX) {
					fprintf(stderr, "Invalid value for fps-init: %s\n", optarg);
					opterr_flag = 1;
				} else {
					dfps_init = n;
				}
				break;
			case 'm':
				n = strtol(optarg, &p, 10);
				if (errno || *p || n < DFPS_MIN || n > DFPS_MAX) {
					fprintf(stderr, "Invalid value for fps-max: %s\n", optarg);
					opterr_flag = 1;
				} else {
					dfps_max = n;
				}
				break;
			case 'j':
				n = strtol(optarg, &p, 10);
				if (errno || *p || n < 0 || n > LENGTH_MAX) {
					fprintf(stderr, "Invalid value for length-init: %s\n", optarg);
					opterr_flag = 1;
				} else {
					length_init = n;
				}
				break;
			case 'n':
				n = strtol(optarg, &p, 10);
				if (errno || *p || n < 0 || n > LENGTH_MAX) {
					fprintf(stderr, "Invalid value for length-max: %s\n", optarg);
					opterr_flag = 1;
				} else {
					length_max = n;
				}
				break;
			case '?':
				opterr_flag = 1;
				break;
		}
	}

	if (dfps_init > dfps_max) {
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
		init_pair(COLOR_HEAD, COLOR_BLUE, bright_flag ? COLOR_BLUE : COLOR_BLACK);
		init_pair(COLOR_BODY, COLOR_CYAN, bright_flag ? COLOR_CYAN : COLOR_BLACK);
		init_pair(COLOR_FOOD, COLOR_GREEN, bright_flag ? COLOR_GREEN : COLOR_BLACK);
		init_pair(COLOR_PORTAL, COLOR_RED, bright_flag ? COLOR_RED : COLOR_BLACK);
	}

	attron(A_BOLD);

	dfps_cur = dfps_init;

	s.head = s.tail = fetch_block();

	s.head->p.x = COLS / 2;
	s.head->p.y = LINES / 2;

	extend_snake(&s, length_init, length_max);

	place_food(COLS, LINES, s, &food, &portal);
	redraw(s.head, food, portal, instructions_flag);

	s.dir = NORTH + rand() % 4;

	time_start = time_stop = time(NULL);

	enum direction next_dir = NO_DIR;

	while (s.dir != DEAD) {
		struct timeval until_next;
		unsigned int sleep_time;

		int update = 0;
		int rev = 0;

		struct timeval now;
		gettimeofday(&now, NULL);

		if (timercmp(&now, &next_frame, >)) {
			sleep_time = dfps_to_delay(dfps_cur) * (is_snake_vertical(s) ? V_COEF : H_COEF);

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
					next_dir = NORTH;
					break;
				case KEY_RIGHT:
				case KEY_RIGHT_ALT:
					next_dir = EAST;
					break;
				case KEY_DOWN:
				case KEY_DOWN_ALT:
					next_dir = SOUTH;
					break;
				case KEY_LEFT:
				case KEY_LEFT_ALT:
					next_dir = WEST;
					break;
				case KEY_PAUSE:
					paused = !paused;
					if (paused) {
						print_instructions();
						mvaddstr(s.head->p.y, s.head->p.x, "PAUSED");
					} else {
						redraw(s.head, food, portal, instructions_flag);
					}

					break;
				case KEY_QUIT:
					cause_of_death = DEATH_QUIT;

					cleanup(s, time_start, time_stop, dfps_cur, cause_of_death, 0);
					return 0;
				case KEY_RESIZE:
					if (s.head->p.x > COLS)
						s.head->p.x = COLS - 1;

					if (s.head->p.y > LINES)
						s.head->p.y = LINES - 1;

					place_food(COLS, LINES, s, &food, &portal);
					redraw(s.head, food, portal, instructions_flag);
					break;
				default:
					break;
			}
		}

		if (!paused && update) {
			if (next_dir != NO_DIR && s.dir != next_dir) {
				if (is_dir_opposite(s.dir, next_dir)) {
					reverse_snake(&s);
					rev = 1;
				}

				dfps_cur = speed_up(dfps_cur, dfps_max, acceleration);
				s.dir = next_dir;
				next_dir = NO_DIR;
			}

			mvaddch(s.tail->p.y, s.tail->p.x, CH_VOID);

			switch (move_snake(COLS, LINES, &s, &food, &portal, length_max)) {
				case 1:
					if (rev)
						cause_of_death = DEATH_REVERSE;
					else
						cause_of_death = DEATH_SELF;
					s.dir = DEAD;
					break;
				case 2:
					cause_of_death = DEATH_PORTAL;
					s.dir = DEAD;
					break;
				case 3:
					redraw(s.head, food, portal, instructions_flag);
				default:
					break;
			}

			if (s.dir != DEAD) {
				do_color(COLOR_HEAD, 1);
				mvaddch(s.head->p.y, s.head->p.x, CH_HEAD);
				do_color(COLOR_HEAD, 0);

				if (s.head->next) {
						do_color(COLOR_BODY, 1);
						mvaddch(s.head->next->p.y, s.head->next->p.x, CH_BODY);
						do_color(COLOR_BODY, 0);
				}
			}
		}

		if (dfps_display_flag) {
			sprintf(dfps_display_buf, "%d", dfps_cur);
			mvaddstr(LINES - 1, 0, dfps_display_buf);
		}

		if (bailing) {
			cleanup(s, time_start, time_stop, dfps_cur, cause_of_death, 0);
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

	cleanup(s, time_start, time_stop, dfps_cur, cause_of_death, 1);
	return 0;
}
