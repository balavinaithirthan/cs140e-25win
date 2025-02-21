#include "rpi-asm.h"

@ save all the general registers and the spsr in 
@ ascending order onto a block of memory on the
@ stack.  the pointer to the base of this to 
@ do_syscall (defined in `2-reg-save.c` and 
@ `2-reg-save-full.c`).
@
@ TODO: why don't we just put everything on the r0??

swi_fn:
    asm_todo("hemfpowefpowefwi\n");

swi_trampoline:
    sub sp, sp, #16
    push {r0-r12} @equivelent to stmia! -> requires sp to be the lowest register (ie no 13 and 14)
    @ add r0, sp, #68 @ 52 + 16 old sp value ?? should this be the old stack
    @ str sp, [r0, #52]
    add r0, sp, #52
    stm r0, {r13, r14}^
    @ str sp, [sp, #52] @ sp, r13
    @ str lr, [sp, #56] @lr, r14
    @ add r0, r0,  @ swi instruction
    str lr, [sp, #60]  @pc, r15
    mrs r0, spsr
    str r0, [sp, #64]
    mov r0, sp
    @ asm_todo("implement this")
    b do_syscall
    @ note: this trampoline does not resume.
    asm_not_reached();

@ only handle single step.
.align 5; 
.globl swi_test_handlers
swi_test_handlers:
    b unhandled_reset
    b unhandled_undefined_instruction
    b swi_trampoline
    b unhandled_prefetch_abort 
    b unhandled_data_abort
    b unhandled_interrupt
    b unhandled_fiq


.weak do_syscall
do_syscall:
    asm_todo("hemfpowefpowefwi\n");
    asm_not_reached();
