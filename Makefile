FLAGS = -std=c99 -Werror -pedantic -Wall -Wextra -ftrapv -ggdb3 
kilo: kilo.c
	$(CC) $(FLAGS) -o kilo kilo.c && ./kilo
