#ifndef __INTERFACE_H
#define __INTERFACE_H

#define CH_HEAD   '@'
#define CH_BODY   '#'
#define CH_FOOD   '*'
#define CH_PORTAL '^'
#define CH_VOID   ' '

#define KEY_LEFT_ALT  'h'
#define KEY_DOWN_ALT  'j'
#define KEY_UP_ALT    'k'
#define KEY_RIGHT_ALT 'l'
#define KEY_PAUSE     'p'
#define KEY_QUIT      'q'

#define H_COEF 2
#define V_COEF 3

#include "things.h"

enum item_color {
	COLOR_HEAD = 1,
	COLOR_BODY,
	COLOR_FOOD,
	COLOR_PORTAL,
};

void do_color(enum item_color c, int on);
void print_instructions();
void redraw(struct block *head, struct posn food, struct posn portal, int instructions_flag);
const char *generate_header();
void show_usage(char *cmd);
int parse_uint_arg(unsigned int *target, char *arg_name, char *arg_value, int min_bound, int max_bound, int *opterr_flag);

#endif /* __INTERFACE_H */
