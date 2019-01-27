CC=gcc
CFLAGS=-std=c99 -Wall
ASM=./utils/assembler
EXEC=icpu

all: simulator-interrupt 4p-os.code

simulator-interrupt: simulator-interrupt.c; $(CC) -o $(EXEC) simulator-interrupt.c $(CFLAGS)

debug: simulator-interrupt.c; $(CC) -o $(EXEC)  simulator-interrupt.c $(CFLAGS) -DDEBUG

%.code: %.asm; $(ASM) $< $@

run:; ./$(EXEC) 4p-os.code 30

clean:; rm icpu *.code
