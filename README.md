# ISA Architecture (CPU simulator and assembler)

This repository contains source code of a CPU simulator and an assembler. The code base was originally an assignment of the computer architecture course by [Prof. Zili Shao](http://www.cse.cuhk.edu.hk/~shao/) in CUHK but I modified most of the code in the CPU simulator (```simulator-interrupt.c```) and added the assembler (```assembler.c```). I hope this repository can provide elementary illustration of CPU pipeline and the principles of two-pass assembler.

## CPU Simulator

The code simulates a CPU that can execute a program (binary code) based on the instruction set listed below. Specifically, inside the CPU, there is a counter and it will be incremented by one in each instruction cycle. In every 5000 cycles, a timer interrupt will be triggered. When a timer interrupt occurs, the corresponding bit of PSR will be set if the interrupt is enabled. At the end of each CPU cycle, we need to check if there is a pending interrupt (if the interrupt bit in PSR is set) and jump to the interrupt handler if the interrupt is enabled.

There are 64 general purpose registers (R0-R63) and one stack pointer register (R64, or sp).

## Assembler

The assembler can assemble assembly source code into assembly binary code. The assemble process is divided into two phases. In phase one, the assembler builds the symbol table and remove comments. In phase two, the assembler substitute symbols with their address respectively, then parse opcode-operand pair and data segments into binary code.

The data segment of this assembly language only support ```.word```. The instruction set is provided in ```instruction.pdf```.



### How to compile?
```
$ make
```

### How to run?
```
$ make run
```

