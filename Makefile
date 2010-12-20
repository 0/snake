CFLAGS = -Wall -Wextra
LDFLAGS = -lcurses

game_sources = $(wildcard *.c)
game_depends = $(game_sources:.c=.dep)
game_objects = $(game_sources:.c=.o)

snake: $(game_objects)

%.dep: %.c
	@set -e; $(CC) -MM $(CFLAGS) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@: ,g' > $@

-include $(game_depends)

.PHONY: all clean

all: snake

clean:
	-rm -f snake $(game_objects) $(game_depends)
