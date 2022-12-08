all:
	cc *.c -Wall -Wextra -pedantic -o json

debug:
	cc *.c -g -Wall -Wextra -pedantic -o json

debug_run:debug
	./json

run:all
	./json
