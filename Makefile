CC=gcc
CFLAGS=-Wall

all: simulator-interrupt

simulator-interrupt: simulator-interrupt.c; $(CC) -o icpu simulator-interrupt.c $(CFLAGS)

clean:; rm icpu
