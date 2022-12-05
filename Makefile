all:
	cc *.c -Wall -Wextra -pedantic -o json

run:all
	./json
