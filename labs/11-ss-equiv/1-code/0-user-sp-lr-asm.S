#include "rpi-asm.h"
    
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ get USER banked regs by switching to SYSTEM mode and back.
@ if we switched to USER, we would need to use a system call to 
@ get back.

@ get the USER sp by switching to SYS_MODE 
@ and back.
@ allows the supervisor mode to get access to the user mode stack
MK_FN(cps_user_sp_get)
    cps #SYS_MODE
    prefetch_flush(r1);

    @ USER And SYSTEM share the same sp/lr/cpsr
    mov r0, sp

    cps #SUPER_MODE
    prefetch_flush(r1);

    bx lr

@ set the USER sp
MK_FN(cps_user_sp_set)
    cps #SYS_MODE
    prefetch_flush(r1); @use a trashed register
    mov sp, r0
    cps #SUPER_MODE @ TODO: needed??
    prefetch_flush(r1);
    bx lr

@ get the USER lr 
MK_FN(cps_user_lr_get)
    cps #SYS_MODE
    prefetch_flush(r1);
    mov r0, lr
    cps #SUPER_MODE
    prefetch_flush(r1);
    bx lr

@ set USER lr 
MK_FN(cps_user_lr_set)
    cps #SYS_MODE
    prefetch_flush(r1);
    mov lr, r0
    cps #SUPER_MODE
    prefetch_flush(r1);
    @asm_todo(implement this using a macro to wrap things up);
    bx lr

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ get USER banked regs by using ldm/stm instructions with the ^
@ modifier.

@ address to a uint32_t word is passed in r0
MK_FN(mem_user_sp_get)
    @ dereference the address held in <r0>
    @ and write the 32-bit result into user
    @ mode <sp> register.
    stm r0, {sp}^
    bx lr

@ address to a uint32_t word is passed in r0
MK_FN(mem_user_sp_set)
    ldm r0, {sp}^
    bx lr

@ address to a uint32_t word is passed in r0
MK_FN(mem_user_lr_get)
    stm r0, {lr}^
    bx lr

@ address to a uint32_t word is passed in r0
MK_FN(mem_user_lr_set)
    ldm r0, {lr}^
    bx lr

@ address to an array with two uint32_t words 
@ is passed in r0
MK_FN(mem_user_sp_lr_get)
    stm r0, {sp, lr}^
    bx lr

@ address to an array with two uint32_t words 
@ is passed in r0
MK_FN(mem_user_sp_lr_set)
    ldm r0, {sp, lr}^
    bx lr

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ remember that registers are like temporary variables and permanant stuff needs to be placed in memory with a store
@ switch to a given mode to get the values we want.
@ 
@ void mode_get_lr_sp_asm(mode, &sp, &lr)
@    - mode in r0
@    - address to hold sp in r1
@    - address to hold lr in r2
MK_FN(mode_get_lr_sp_asm)
    mrs r3, cpsr @just the mode bits
    msr cpsr_c, r0
    @prefetch_flush(r3) @ TODO: WHY IS THIS STALLING???
    str sp, [r1] @ remember that registers are temporary values and you need to store it back into memory. you have to move it back!!!!!!!!
    str lr, [r2]
    msr cpsr, r3
    @prefetch_flush(r3);
    bx lr

