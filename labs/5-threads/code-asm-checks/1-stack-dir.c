// write code in C to check if stack grows up or down.
// suggestion:
//   - local variables are on the stack.
//   - so take the address of a local, call another routine, and
//     compare addresses of one of its local variables to the
//     original.
//   - make sure you check the machine code make sure the
//     compiler didn't optimize the calls away!
#include "rpi.h"

int dummy_fn(void) {
  unsigned int stackptr;
  asm volatile("mov %0, sp" : "=r"(stackptr));
  printk("stack pointer is %d \n", stackptr);
  return stackptr;
}

int stack_grows_down(void) {
  unsigned int stackptr;
  asm volatile("mov %0, sp" : "=r"(stackptr));
  printk("stack pointer is %d \n", stackptr);
  if (dummy_fn() < stackptr) {
    return 1;
  }
  return 0;
}

void notmain(void) {
  if (stack_grows_down())
    trace("stack grows down\n");
  else
    trace("stack grows up\n");
}
