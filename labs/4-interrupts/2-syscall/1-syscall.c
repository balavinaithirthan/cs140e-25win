// do a system call from user level.
//   can check cpsr to see that the 
// you have to write:
//  - syscall-asm.S:run_user_code_asm to switch to USER mode.
//  - syscall_vector:(below) to check that it came from USER level.
//  - notmain:(below) to setup stack and install handlers.
//  - _int_table_user[], _int_table_user_end[] tables in interrupt-asm.S
#include "rpi.h"
#include "rpi-interrupts.h"

// in syscall-asm.S
void run_user_code_asm(void (*fn)(void), void *stack);

// run <fn> at user level: <stack> must be 8 byte
// aligned
void run_user_code(void (*fn)(void), void *stack) {
    assert(stack);
    demand((unsigned)stack % 8 == 0, stack must be 8 byte aligned);

    // you have to implement <syscall-asm.S:run_user_code_asm>
    // will call <user_fn> with stack pointer <sp>
    printk("stack addreess is %p\n", stack);
    printk("function address is %p\n", fn);
    run_user_code_asm(fn, stack);
    not_reached();
}

// example of using inline assembly to get the cpsr
// you can also use assembly routines.
static inline uint32_t cpsr_get(void) {
    uint32_t cpsr;
    asm volatile("mrs %0,cpsr" : "=r"(cpsr));
    return cpsr;
}

static inline uint32_t spsr_get(void) {
    uint32_t spsr;
    asm volatile("mrs %0,spsr" : "=r"(spsr));
    return spsr;
}

static inline void stack_get(void) {
    uint32_t sp;
    asm volatile ("mov %0, sp" : "=r"(sp)); 
    printk("current stackptr = %x\n", sp);
}

enum { N = 1024 * 64 };
static uint64_t stack[N];

/*********************************************************************
 * modify below.
 */

// this should be running at user level: you don't have to change this
// routine.
void user_fn(void) {
    uint64_t var;
    printk("address of var is %p\n", &var);
    trace("checking that stack got switched\n");
    stack_get();
    assert(&var >= &stack[0]);
    assert(&var < &stack[N]);


    // use <cpsr_get()> above to get the current mode and check that its
    // at user level.
    unsigned mode = 0;
    mode = cpsr_get() & 0x1F;


    if(mode != USER_MODE)
        panic("mode = %b: expected %b\n", mode, USER_MODE);
    else
        trace("cpsr is at user level\n");



    void syscall_hello(void);
    trace("about to call hello\n");
    syscall_hello();

    trace("about to call exit\n");
    void syscall_illegal(void);
    syscall_illegal();

    not_reached();
}


// pc should point to the system call instruction.
//      can see the encoding on a3-29:  lower 24 bits hold the encoding.
int syscall_vector(unsigned pc, uint32_t r0) {
    uint32_t inst, sys_num, mode;
    inst=*(unsigned *)pc;
    sys_num = inst & 0x00FFFFFF;
 
    // make a spsr_get() using cpsr_get() as an example.
    // extract the mode bits and store in <mode>
    //todo("get <spsr> and check that mode bits = USER level\n");
    mode = spsr_get() & 0x1F;

    // do not change this code!
    if(mode != USER_MODE)
        panic("mode = %b: expected %b\n", mode, USER_MODE);
    else
        trace("success: spsr is at user level: mode=%b\n", mode);

    // we do a very trivial hello and exit just to show the difference
    switch(sys_num) {
    case 1: 
            trace("syscall: hello world\n");
            return 0;
    case 2: 
            trace("exiting!\n");
            clean_reboot();
    default: 
            printk("illegal system call = %d!\n", sys_num);
            return -1;
    }
}



void notmain() {
    // define a new interrupt vector table, and pass it to 
    // rpi-interrupts.h:interrupt_init_v
    // NOTE: make sure you set the stack pointer.
    // todo("use rpi-interrupts.h:<interrupt_init_v> (in this dir) "
    //      "to install a interrupt vector with a different swi handler");
    extern uint32_t _interrupt_table2[];
    extern uint32_t _interrupt_table_end2[];
    interrupt_init_v(_interrupt_table2, _interrupt_table_end2);

    // use the <stack> array above.  note: stack grows down.
    uint64_t *sp = (uint64_t*)((char*)stack + ((N-1) * 8));

    output("calling user_fn with stack=%p\n", sp);
    run_user_code(user_fn, sp); 
    printk("hfemwipfpowfopewfopewfopewfopwekfopewpkoewkfopwekfpoi");
    not_reached();
}
