CC=gcc
CFLAGS=-I.
DEPS = project1.h
run: project1
	./project1

%.o: %.c $(DEPS)
	$(CC) -w -c -o $@ $< $(CFLAGS)

project1: project1.o
	gcc -w -o project1 project1.o -I.
