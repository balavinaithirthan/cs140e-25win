#include "rpi-asm.h"

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@
@ <0-rfe-example.c>: we give you this code.

@ use rfe to load <pc> and <cpsr> from the 2 entry
@ array passed in <r0>.
MK_FN(rfe_asm)
    mov sp, r0 @ TODO: why mov to the stack pointer the regs-> oh place new stack with all registers on top of it
    rfe sp
    asm_not_reached();

@ a trampoline that loads the stack pointer
@ and calls <rfe_c_code>
MK_FN(rfe_trampoline_asm)
    mov sp, #INT_STACK_ADDR
    mov r0, sp 
    bl rfe_c_code
    asm_not_reached();

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@
@ <1-rfe-blk.c>

@ tiny change: modify rfe_asm to deal with being 
@ passed a 17-entry register array
@ r0 is the addr of regs array, so should be r0 + 15 is the pointer to starting
MK_FN(blk_rfe_asm)
    mov sp, r0
    add sp, sp, $60
    rfe sp
    asm_not_reached();

@ do not need to modify: same as original, so just
@ call it.
MK_FN(blk_trampoline_asm)
    bl rfe_trampoline_asm
    asm_not_reached();


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@
@ <1-rfe-load-sp.c>

@ 
@ copy-paste <blk_rfe_asm> above and change it 
@ so it:
@  1. loads r0-r14 into user mode registers
@     (hint: look back at your <0-user-sp-lr> 
@     code)
@  2. does an rfe to load pc and cpsr
MK_FN(switchto_user_asm)
    
    @ mov r1, r0
    @ ldm r0, {r14}^ @load user regs into current
    @ ldr r1, r0
    @ ldr 
    @ add r0, r0, $52 @ new SP
    add sp, r0, $60
    add r0, r0, $4
    ldm r0, {r1-r12}
    add r0, r0, $48
    ldm r0, {r13-r14}^
    sub r0, r0, #52
    ldr r0, [r0]
    rfe sp
    asm_not_reached();
    @ TODO: hella weird issues here, why can't I just ldr r0 - r14
@ so we don't get linker errors 
.weak rfe_c_code
rfe_c_code:
    asm_not_reached()
