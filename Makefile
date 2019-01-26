CC=gcc
CFLAGS=-std=c99 -Wall

all: simulator-interrupt 4p-os

simulator-interrupt: simulator-interrupt.c; $(CC) -o icpu simulator-interrupt.c $(CFLAGS)

debug: simulator-interrupt.c; $(CC) -o icpu simulator-interrupt.c $(CFLAGS) -DDEBUG

4p-os: 4p-os.asm; ./utils/assembler 4p-os.asm 4p-os.code

clean:; rm icpu
