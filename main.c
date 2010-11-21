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

#define H_COEF 2
#define V_COEF 3

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

void cleanup(struct snake s, time_t time_start, time_t time_stop, unsigned int frame_wait, enum cod cause_of_death, int proper_exit) {
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

	struct snake s = {NULL, NULL, 0, DEAD};

	struct posn food = {-1, -1};
	struct posn portal;

	unsigned int frame_wait;
	unsigned int frame_min;

	enum cod cause_of_death = DEATH_UNKNOWN;

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

	placeFood(COLS, LINES, s, &food, &portal);
	redraw(s.head, food, portal);

	s.dir = NORTH + rand() % 4;

	time_start = time_stop = time(NULL);

	while (s.dir != DEAD) {
		struct timeval until_next;
		unsigned int sleep_time;

		int update = 0;
		int rev = 0;

		struct timeval now;
		gettimeofday(&now, NULL);

		if (timercmp(&now, &next_frame, >)) {
			sleep_time = frame_wait * (isSnakeVertical(s) ? V_COEF : H_COEF);

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
					if (SOUTH == s.dir) {
						reverseSnake(&s);
						rev = 1;
					}
					s.dir = NORTH;
					frame_wait = speedUp(frame_wait, frame_min);
					break;
				case KEY_RIGHT:
				case KEY_RIGHT_ALT:
					if (WEST == s.dir) {
						reverseSnake(&s);
						rev = 1;
					}
					s.dir = EAST;
					frame_wait = speedUp(frame_wait, frame_min);
					break;
				case KEY_DOWN:
				case KEY_DOWN_ALT:
					if (NORTH == s.dir) {
						reverseSnake(&s);
						rev = 1;
					}
					s.dir = SOUTH;
					frame_wait = speedUp(frame_wait, frame_min);
					break;
				case KEY_LEFT:
				case KEY_LEFT_ALT:
					if (EAST == s.dir) {
						reverseSnake(&s);
						rev = 1;
					}
					s.dir = WEST;
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

					placeFood(COLS, LINES, s, &food, &portal);
					redraw(s.head, food, portal);
					break;
				default:
					break;
			}
		}

		if (!paused && update) {
			mvaddch(s.tail->p.y, s.tail->p.x, CH_VOID);

			switch (moveSnake(COLS, LINES, &s, &food, &portal, length_max)) {
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
					redraw(s.head, food, portal);
				default:
					break;
			}

			if (s.dir != DEAD) {
				do_color(COLOR_LEAD, 1);
				mvaddch(s.head->p.y, s.head->p.x, CH_HEAD);
				do_color(COLOR_LEAD, 0);

				if (s.head->next) {
						do_color(COLOR_BLOCK, 1);
						mvaddch(s.head->next->p.y, s.head->next->p.x, CH_BODY);
						do_color(COLOR_BLOCK, 0);
				}
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
