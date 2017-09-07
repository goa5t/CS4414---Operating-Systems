CC=gcc
CFLAGS=-I.
DEPS = project2.h

%.o: %.c $(DEPS)
	$(CC) -w -c -o $@ $< $(CFLAGS)

project2: project2.o
	gcc -w -o project2 project2.o -I. -lpthread
