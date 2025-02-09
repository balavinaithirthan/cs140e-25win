// very dumb, simple interface to wrap up watchpoints better.
// only handles a single watchpoint.
//
// You should be able to take most of the code from the 
// <1-watchpt-test.c> test case where you already did 
// all the needed operations.  This interface just packages 
// them up a bit.
//
// possible extensions:
//   - do as many as hardware supports, perhaps a handler for 
//     each one.
//   - make fast.
//   - support multiple independent watchpoints so you'd return
//     some kind of structure and pass it in as a parameter to
//     the routines.
#include "armv6-debug-impl.h"
#include "rpi.h"
#include "mini-watch.h"
#include "full-except.h"
#include <stdint.h>

// we have a single handler: so just use globals.
static watch_handler_t watchpt_handler = 0;
static void *watchpt_data = 0;

// is it a load fault?
static int mini_watch_load_fault(void) {
    todo("implement");
}

// if we have a dataabort fault, call the watchpoint
// handler.
static void watchpt_fault(regs_t *r) {
    // watchpt handler.
    if(was_brkpt_fault())
        panic("should only get debug faults!\n");
    if(!was_watchpt_fault())
        panic("should only get watchpoint faults!\n");
    if(!watchpt_handler)
        panic("watchpoint fault without a fault handler\n");

    watch_fault_t w = {0};

    todo("setup the <watch_fault_t> structure");
    todo("call: watchpt_handler(watchpt_data, &w);");

    // in case they change the regs.
    switchto(w.regs);
}

// setup:
//   - exception handlers, 
//   - cp14, 
//   - setup the watchpoint handler
// (see: <1-watchpt-test.c>
void mini_watch_init(watch_handler_t h, void *data) {
    full_except_install(0);
    full_except_set_data_abort(watchpt_fault);

    //uint32_t debug_id_macro = 0;

    // check that bits 0..31 of ~0 are 1.
    cp14_enable();
    assert(cp14_is_enabled());
    // just started, should not be enabled.
    assert(!cp14_bcr0_is_enabled());
    assert(!cp14_wcr0_is_enabled());

    watchpt_handler = h;
    watchpt_data = data;
}

// set a watch-point on <addr>.
void mini_watch_addr(void *addr) { // TODO: why make this a void *, addr if just a number right??
    uint32_t b = 0;
    b = bits_set(b, 20, 20, 0); // disable linking
    b = bits_set(b, 14, 15, 0); // Watchpoint match
    b = bits_set(b, 5, 8, 15); // Byte addr select, make sure 4 bits match
    b = bits_set(b, 3, 4, 3); // load/access store
    b = bits_set(b, 1, 2, 3); // priv for access 
    if(!b)
        panic("set b to the right bits for wcr0\n");

    cp14_wcr0_set(b);
    cp14_wcr0_enable();
    cp14_wvr0_set((uint32_t)addr); // (uint32_t)(uintptr_t)addr;
    assert(cp14_wcr0_is_enabled());
}

// disable current watchpoint <addr>
void mini_watch_disable(void *addr) {
    // cp14_wcr0_disable();
    todo("implement");
}

// return 1 if enabled.
int mini_watch_enabled(void) {
    return cp14_wcr0_is_enabled();
}

// called from exception handler: if the current 
// fault is a watchpoint, return 1
int mini_watch_is_fault(void) { 
    todo("implement");
}
