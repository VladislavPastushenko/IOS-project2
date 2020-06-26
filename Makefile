all:
	gcc proj2.c -std=gnu99 -Wall -Wextra -Werror -pedantic -lpthread -lrt -o proj2
clean:
	rm proj2
	rm proj2.out
test:
	./proj2 5 2 7 1 1
	cat proj2.out