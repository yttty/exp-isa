;;; .data
.word 0                         ; ISR address

;;; stack for 2 processes
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
A_stack_top:

.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
B_stack_top:

current:
.word 0                         ; current proc num
process_control_block:
.word 0                         ; saved sp(A)
.word 0                         ; saved sp(B)

;;; .text
OS_start:
    ;; setup ISR
    movei   R0, 0
    la      R1, save_context
    sw      R0, R1, 0

    ;; setup initial context for Process B
    la      sp, B_stack_top     ; sp <- B_stack_top
    ;; save PSR and PC
    movei   R1, 1               ; bit 1: Interrupt Enabled
    push    R1
    la      R1, proc_start      ; initial PC points to process entry point
    push    R1

    ;; save(clear) general purpose registers
    movei   R1, 0
    push    R1			; Push for R0
    push    R1			; Push for R1
    push    R1			; Push for R2
    movei   R3, 66              ; R3 <- 'B'
    push    R3                  ; save R3 so that after restoring the context, proc A should see 'A' and proc B should see 'B'

    ;; save sp 
    la      R2, process_control_block
    addi    R2, R2, 1		; sp is saved to process_control_block[1] for context switch 
    sw      R2, sp, 0

    ;; Execute Process A first
    ;;   First, set up the current process id (0) for Process A 
    movei   R0, 0		
    la      R1, current 
    sw      R1, R0, 0
    ;;   Second, set up the inital value of R3 (for print 'A') 
    movei  R3, 65		; R3 <- 'A'
    ;;   Third, set up sp of Process A
    la     sp, A_stack_top
    ;;   Finally, use iret to return back to user
    movei   R1, 1               ; bit 1: Interrupt Enabled
    push    R1
    la      R1, proc_start      ; initial PC points to process entry point
    push    R1
    iret

save_context:
    ;; save general purpose registers
    push    R0
    push    R1
    push    R2
    push    R3

    la      R0, current
    lw      R0, R3, 0           ; R3 <- current process id

    ;; save sp
    la      R0, process_control_block 
    add     R3, R0              ; R0 <- addr of process_control_block[current]
    sw      R0, sp, 0           ; process_control_block[current] <- sp
    ;; set R2 as the next process id
    movei    R2, 1              ; if R3 == 0 then R2 <- 1 else 0
    blez    R3, switch_proc
    movei    R2, 0
switch_proc:                 ; R2 = next process id
    ;; update current process id
    la      R0, current
    sw      R0, R2, 0           ; current <- R2
    ;; restore sp
    la      R0, process_control_block 
    add     R2, R0              ; R0 <- addr of process_control_block[next]
    lw      R0, sp, 0           ; sp <- process_control_block[next]
    ;; restore general purpose registers
    pop     R3
    pop     R2
    pop     R1
    pop     R0
    ;; go back to user
    iret

proc_start:                     ; entry point of user process
    put     R3                  ; print the char in R3 on the screen 
    jmp     proc_start
