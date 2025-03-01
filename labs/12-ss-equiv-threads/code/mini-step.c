// very simple code to just run a single function at user level 
// in mismatch mode.  
//
// search for "todo" and fix.
#include "rpi.h"
#include "/Users/balavinaithirthan/Main/CS/cs140e-25win/labs/9-debug-hw/code/armv6-debug-impl.h"
// #include "armv6-debug-impl.h"
// #include "mini-step.h"
#include "mini-step.h"
#include <stdint.h>
// #include "full-except.h"

// currently only handle a single breakpoint.
static step_handler_t step_handler = 0;
static void *step_handler_data = 0;

// special function.  never runs, but we know if the traced code
// calls it, that they are done.
void ss_on_exit(int exitcode);

// error checking.
static int single_step_on_p;

// registers where we started single-stepping
static regs_t start_regs;

// 0. get the previous pc that we were mismatching on.
// 1. set bvr0/bcr0 for mismatch on <pc>
// 2. prefetch_flush();
// 3. return old pc.
uint32_t brkpt_mismatch_set(uint32_t pc) {
    // assert(single_step_on_p);
    uint32_t old_pc = cp14_bvr0_get();

    // set a mismatch (vs match) using bvr0 and bcr0 on
    // <pc>
    uint32_t b = 0;
    b = bits_set(b, 21, 22, 2); // TODO: what does execution context mean??
    // breakpoint only occurs when PC goes to IMVA/addr vs watch point is read/write to an addr
    b = bits_set(b, 20, 20, 0); // disable linking
    b = bits_set(b, 14, 15, 2); // Breakpoint secure
    b = bits_set(b, 5, 8, 15); // Byte addr select, make sure 4 bits match
    // b = bits_set(b, 3, 4, 3); // load/access store
    b = bits_set(b, 1, 2, 3); // user mode 
    cp14_bcr0_set(b);
    cp14_bvr0_set((uint32_t)pc);
    cp14_bcr0_enable();
    prefetch_flush(); // should only occur in user mode
    assert(cp14_bcr0_is_enabled()); // why doesn't this like break??
    assert( cp14_bvr0_get() == pc);
    return old_pc;
}

// turn mismatching on: should only be called 1 time.
void brkpt_mismatch_start(void) {
    if(single_step_on_p)
        panic("mismatch_on: already turned on\n");
    single_step_on_p = 1;
    // is ok if we call more than once.
    cp14_enable();


    // we assume they won't jump to 0.
    brkpt_mismatch_set(0); // mismatch to 0 -> should hit breakpoint
}

// disable mis-matching by just turning it off in bcr0
void brkpt_mismatch_stop(void) {
    if(!single_step_on_p)
        panic("mismatch not turned on\n");
    single_step_on_p = 0;

    uint32_t b = 0;
    cp14_bcr0_disable();
    b = cp14_bcr0_get();
    b = bits_set(b, 21, 22, 1); // TODO: what does execution context mean??
    cp14_bcr0_set(b);
    cp14_bcr0_enable();
    prefetch_flush(); 
    assert(cp14_bcr0_is_enabled()); 

    // RMW bcr0 to disable breakpoint, 
    // make sure you do a prefetch_flush!
    // todo("turn mismatch off, but don't modify anything else");
}

// set a mismatch fault on the pc register in <r>
// context switch to <r>
void mismatch_run(regs_t *r) {
    uint32_t pc = r->regs[REGS_PC];

    brkpt_mismatch_set(pc);

    // otherwise there is a race condition if we are
    // stepping through the uart code --- note: we could
    // just check the pc and the address range of
    // uart.o
    while(!uart_can_put8())
        ;

    // load all the registers in <r> and run
    switchto(r);
}

// the code you trace should call this routine
// when its done: the handler checks if the <pc>
// register in the exception equals <ss_on_exit>
// and if so stops the code.
void ss_on_exit(int exitcode) {
    panic("should never reach this!\n");
}


int brkpt_fault_p() {
    if(!was_brkpt_fault()) {
        panic("should only get a breakpoint fault\n");
    }
    assert(cp14_bcr0_is_enabled()); 
    return 1;
}

// when we get a mismatch fault.
// 1. check if its for <ss_on_exit>: if so
//    switch back to start_regs.
// 2. otherwise setup the fault and call the
//    handler.  will look like 2-brkpt-test.c
//    somewhat.
// 3. when returns, set the mismatch on the 
//    current pc.
// 4. wait until the UART can get a putc before
//    return (if you don't do this what happens?)
// 5. use switch() to to resume.
static void mismatch_fault(regs_t *r) {
    uint32_t pc = r->regs[15];

    // example of using intrinsic built-in routines
    if(pc == (uint32_t)ss_on_exit) {
        output("done pc=%x: resuming initial caller\n", pc);
        switchto(&start_regs);
        not_reached();
    }

    step_fault_t f = {};
    if(!was_brkpt_fault())
        panic("should only get a breakpoint fault\n");
    assert(cp14_bcr0_is_enabled());

    // 13-34: effect of exception on watchpoint registers.
    cp14_bcr0_disable();
    assert(!cp14_bcr0_is_enabled());
    step_fault_t t = {pc, r};
    step_handler(step_handler_data, &t);
    brkpt_mismatch_set(pc);
    // todo("setup fault handler and call step_handler");
    // todo("setup a mismatch on pc");

    // otherwise there is a race condition if we are 
    // stepping through the uart code --- note: we could
    // just check the pc and the address range of
    // uart.o
    while(!uart_can_put8()) // TODO: why
        ;

    switchto(r);
}

// will look like <mini_watch_init> but for
// breakpoints (not watchpoints) with prefetch
// exception.
void mini_step_init(step_handler_t h, void *data) {
    assert(h);
    step_handler_data = data;
    step_handler = h;
    
    full_except_install(0);
    full_except_set_prefetch(mismatch_fault);

    // 2. enable the debug coprocessor.
    cp14_enable();

    // just started, should not be enabled.
    assert(!cp14_bcr0_is_enabled());


    // just started, should not be enabled.
    assert(!cp14_bcr0_is_enabled());
    assert(!cp14_bcr0_is_enabled());
}

// run <fn> with argument <arg> in single step mode.
uint32_t mini_step_run(void (*fn)(void*), void *arg) {
    uint32_t pc = (uint32_t)fn;

    // we use the same stack at the same address so the 
    // values check out from run to run.
    static void *stack = 0;
    enum { stack_size = 8192*4};
    
    if(!stack)
        stack = kmalloc(stack_size);

    void *sp = stack + stack_size;

    // take the current cpsr and switch to user mode and clear
    // carry flags.
    uint32_t cpsr = cpsr_inherit(USER_MODE, cpsr_get());
    assert(mode_get(cpsr) == USER_MODE);

    // setup our initial registers.  everything not set will be 0.
    regs_t r = (regs_t) {
        .regs[REGS_PC] = (uint32_t)fn,      // where we want to jump to
        .regs[REGS_SP] = (uint32_t)sp,      // the stack pointer to use.
        .regs[REGS_LR] = (uint32_t)ss_on_exit, // where to jump if return.
        .regs[REGS_CPSR] = cpsr             // the cpsr to use.
    };

    // note: we won't fault b/c we are not at user level yet!
    brkpt_mismatch_start();
    // switch to <r> save values in <start_regs>
    switchto_cswitch(&start_regs, &r);

    // mistmatch is off.
    brkpt_mismatch_stop();

    // return what the user set.
    return r.regs[0];
}
