#ifndef __THINGS_H
#define __THINGS_H

struct posn {
	int x;
	int y;
};

struct block {
	struct posn p;
	struct block *next;
	struct block *prev;
};

struct block *fetch_block();

#endif /* __THINGS_H */
