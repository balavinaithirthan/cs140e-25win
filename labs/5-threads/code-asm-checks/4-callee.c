// NOTE: you need to implement <todo> and handle all
// registers from r0-r12
//
// we do some sleazy tricks to mechanically (automatically)
// verify which registers r0-r12 gcc thinks are caller-saved
// and which are callee-saved.
//
// intuition:
//    we know from the <notes/caller-callee/README.md> that
//    if a routine contains a single gcc's inline assembly
//    statement that "clobbers" a register (i.e., tells gcc
//    that the assembly modifies it) that gcc will save and
//    restore the register iff it's callee-saved and do
//    nothing (just return) if it's caller-saved.
//
#include "rpi.h"

// write <is_empty>: given a routine address <fp>, return 1 if
// <fp> does nothing but return.
//  1. figure out what machine instruction should be the first
//     (only) instruction in the routine.
//  2. figure out how to get the first instruction given
//     a function address.
//  3. return 1 if (2) == (1).
//
// we use this to tell if a register <r> is caller saved:
//   1. generate a routine that only clobbers <r>
//   2. if routine is empty, <r> was a caller reg.
//   3. if routine is not empty, <r> was a callee reg.
static inline int is_empty(void (*fp)(void)) {
  unsigned value;
  asm volatile("mov r3, %0" ::"r"(fp));
  asm volatile("ldr r3, [r3]");
  asm volatile("mov r4, #8");
  asm volatile("mov %0, r3" : "=r"(value));
  // printk("value of bx lr is %u \n", value);
  if (value == 0xE12FFF1E) { // op code for bx lr
    return 1;                // is empty
  }
  return 0;
  // todo("returns 1 if routine does nothing besides return");
}

#define CLOBBER_REG(reg)                                                       \
  static void clobber_##reg(void) { asm volatile("" : : : #reg); }
// note: you can do these better with macros.
// static void clobber_r0(void) { asm volatile("" : : : "r0"); }
// static void clobber_r1(void) { asm volatile("" : : : "r1"); }
CLOBBER_REG(r0);
CLOBBER_REG(r1);
CLOBBER_REG(r2);
CLOBBER_REG(r3);
CLOBBER_REG(r4);
CLOBBER_REG(r5);
CLOBBER_REG(r6);
CLOBBER_REG(r7);
CLOBBER_REG(r8);
CLOBBER_REG(r9);
CLOBBER_REG(r10);
CLOBBER_REG(r11);
CLOBBER_REG(r12);

// FILL this in with all caller-saved registers.
// these are all the registers you *DO NOT* save during
// voluntary context switching // TODO: WHY NOT callee saved
//
// NOTE: we know we have to save r13,r14,r15 so ignore
// them.
#define ASSERT_EMPTY(func) assert(is_empty(func));

void check_cswitch_ignore_regs(void) {
  // assert(is_empty(clobber_r0));
  ASSERT_EMPTY(clobber_r0);
  ASSERT_EMPTY(clobber_r1);
  ASSERT_EMPTY(clobber_r2);
  ASSERT_EMPTY(clobber_r3);
  ASSERT_EMPTY(clobber_r12);

  // if you reach here it passed.
  trace("ignore regs passed\n");
}

#define ASSERT_NOT_EMPTY(func) assert(!is_empty(func));

// put all the regs you *do* save during context switching
// here [callee saved]
//
// NOTE: we know we have to save r13,r14,r15 so ignore
// them.
void check_cswitch_save_regs(void) {
  ASSERT_NOT_EMPTY(clobber_r4);
  ASSERT_NOT_EMPTY(clobber_r5);
  ASSERT_NOT_EMPTY(clobber_r6);
  ASSERT_NOT_EMPTY(clobber_r7);
  ASSERT_NOT_EMPTY(clobber_r8);
  ASSERT_NOT_EMPTY(clobber_r9);
  ASSERT_NOT_EMPTY(clobber_r10);
  ASSERT_NOT_EMPTY(clobber_r11);

  trace("saved regs passed\n");
}

void notmain() {
  check_cswitch_ignore_regs();
  check_cswitch_save_regs();
  trace("SUCCESS\n");
}
