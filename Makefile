CC=gcc
CFLAGS=-std=c99 -Wall -g

all: simulator-interrupt

simulator-interrupt: simulator-interrupt.c; $(CC) -o icpu simulator-interrupt.c $(CFLAGS)

clean:; rm icpu