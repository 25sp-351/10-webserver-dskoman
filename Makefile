all: main

CC = gcc
CFLAGS = -Wall -g

main: main.o
	$(CC) -o main.out $(CFLAGS) main.o -lm

main.o: main.c