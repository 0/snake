CFLAGS = -lcurses

all: snake

snake: snake.c

clean:
	-rm -f snake

.PHONY: all clean
