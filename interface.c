#include <curses.h>
#include <errno.h>
#include <stdlib.h>

#include "game.h"
#include "interface.h"

extern int use_color;

void do_color(enum item_color c, int on) {
	if (use_color) {
		if (on) {
			attron(COLOR_PAIR(c));
		} else {
			attroff(COLOR_PAIR(c));
		}
	}
}

void print_instructions() {
	attroff(A_BOLD);
	mvaddstr(0, 0, "move:  arrows or HJKL");
	mvaddstr(1, 0, "stop:  Q or die");
	mvaddstr(2, 0, "pause: P");
	mvaddstr(3, 0, "quit:  any key");
	attron(A_BOLD);
}

void redraw(struct block *head, struct map m, struct posn food, struct posn portal, int instructions_flag) {
	unsigned int i, j;
	struct block *b;

	erase();

	if (instructions_flag)
		print_instructions();

	do_color(COLOR_WALL, 1);
	for (i = 0; i < m.height; i++) {
		for (j = 0; j < m.width; j++) {
			if (CH_WALL == m.tiles[i][j])
				mvaddch(i, j, CH_WALL);
		}
	}
	do_color(COLOR_WALL, 0);

	do_color(COLOR_HEAD, 1);
	mvaddch(head->p.y, head->p.x, CH_HEAD);
	do_color(COLOR_HEAD, 0);

	b = head->next;
	do_color(COLOR_BODY, 1);
	while (b) {
		mvaddch(b->p.y, b->p.x, CH_BODY);
		b = b->next;
	}
	do_color(COLOR_BODY, 0);

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
"    --dfps-display\n"
"        Show current framerate. Default off.\n");

	printf(
"  Gameplay\n"
"    --acceleration <num>\n"
"        Rate of framerate increase (integer). Default: %d\n"
"    --dfps-init <num>\n"
"        Initial framerate (integer). Default: %d\n"
"    --dfps-max <num>\n"
"        Maximum framerate (integer). Default: %d\n"
"    --length-init <num>\n"
"        Initial snake length (integer). Default: 0\n"
"    --length-max <num>\n"
"        Maximum snake length (integer). Default: %d\n"
"    --outside-wall\n"
"        Put up a perimeter wall. Default off.\n",
		ACCEL_DEFAULT, DFPS_INIT, DFPS_MAX, LENGTH_MAX);

	printf(
"  Miscellaneous\n"
"    --help\n"
"        Print this and exit.\n"
"    --no-<foo>\n"
"        Disable boolean option <foo>.\n");

	printf(
"\n"
"VALUES:\n"
"  Acceleration: %d to %d (dfps/turn)\n"
"  Framerate: %d to %d (dfps)\n"
"  Length: 0 to %d\n",
		ACCEL_MIN, ACCEL_MAX, DFPS_MIN, DFPS_MAX, LENGTH_MAX);
}

int parse_uint_arg(unsigned int *target, char *arg_name, char *arg_value, int min_bound, int max_bound, int *opterr_flag) {
	char *p;
	long n;

	errno = 0;
	n = strtol(arg_value, &p, 10);

	if (errno || *p || n < min_bound || n > max_bound) {
		fprintf(stderr, "Invalid value for %s: %s\n", arg_name, arg_value);
		*opterr_flag = 1;
		return 0;
	} else {
		*target = n;
		return 1;
	}
}
