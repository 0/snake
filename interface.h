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

#include "things.h"

enum item_color {
	COLOR_LEAD = 1,
	COLOR_BLOCK,
	COLOR_FOOD,
	COLOR_PORTAL
};

void do_color(enum item_color c, int on);
void redraw(struct block *head, struct posn food, struct posn portal);
const char *generate_header();
void show_usage(char *cmd);

#endif /* __INTERFACE_H */
