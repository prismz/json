all:
	cc *.c -g -Wall -Wextra -pedantic -o json

run:all
	./json
