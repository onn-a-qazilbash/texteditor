CFLAGS = -std=c99 -Werror -pedantic -Wall -Wextra -ftrapv -ggdb3 
CC=gcc
run: kilo
	./kilo

kilo: kilo.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf kilo 
