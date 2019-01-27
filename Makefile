CC=gcc
CFLAGS=-std=c99 -Wall
ASM=asm
EXEC=icpu

all: simulator-interrupt assembler 4p-os.code

assembler: assembler.c; $(CC) -o $(ASM) assembler.c $(CFLAGS)

simulator-interrupt: simulator-interrupt.c; $(CC) -o $(EXEC) simulator-interrupt.c $(CFLAGS)

debug: simulator-interrupt.c; $(CC) -o $(EXEC)  simulator-interrupt.c $(CFLAGS) -DDEBUG

%.code: %.asm assembler; ./$(ASM) $< $@

run:; ./$(EXEC) 4p-os.code 30

clean:; rm -f $(EXEC) $(ASM) *.code
