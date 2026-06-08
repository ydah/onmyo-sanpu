CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

.PHONY: all clean test

all: onmyo

onmyo: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: onmyo
	./test/run.sh

clean:
	rm -f onmyo src/*.o
