i:	.word  -10
count:  .word  0

start:
	;R0 <- the addr. of i
	movei R0, 0
	;R1 <- the content of i
	lw R0, R1, 0
	
	;R0 <- the addr. of count
	movei R0, 1
	;R2 <- the content of count
	lw R0, R2, 0

loop:
	addi  R1, R1, 1
	addi  R2, R2, 1
	blez  R1, loop

	;Save the result back to count (memeory)
	;R0 <- the addr. of count
	movei R0, 1
	;Mem{R0+0} <- R2 (the value for count)
	sw R0, R2, 0

	halt

