/*
 * Simple test that:
 *  1. sets a watchpoint on null (0).
 *  2. does a store using PUT32(0, val)
 *  3. checks that (2) caused the right fault.
 *  4. does a load using GET32(0)
 *  5. checks that (4) caused a fault and returned val.
 *
 * shows how to use the full_except/switchto interface.
 */
// handle a store to address 0 (null)
#include "rpi.h"
#include "vector-base.h"
#include "armv6-debug-impl.h"
#include "full-except.h"

static int load_fault_n, store_fault_n;

// change to passing in the saved registers.

static void watchpt_fault(regs_t *r) {
    if(was_brkpt_fault())
        panic("should only get debug faults!\n");

    if(!was_watchpt_fault())
        panic("should only get watchpoint faults!\n");

    assert(cp14_wcr0_is_enabled());

    // we get the fault *after* the read or write occurs and
    // the pc value is the next instruction that would run.
    // e.g., a store or a load to a non-pc value expect this
    // to be +4.    if they write to the pc can be anywhere.
    uint32_t pc = r->regs[15]; // TODO, why not link register

    // the actual instruction that caused watchpoint.  pc holds the
    // address of the next instruction.
    uint32_t fault_pc = watchpt_fault_pc();

    trace("faulting instruction at pc=%x, next instruction pc=%x\n",
        fault_pc, pc);

    // currently we just expect it to be +4
    uint32_t expect_pc = pc-4;
    if(fault_pc != expect_pc)
        panic("exception fault pc=%p != watchpt_fault_pc() pc=%p\n", 
            expect_pc, fault_pc);

    // we are only using GET32/PUT32 to load/store.
    if(datafault_from_ld()) {
        trace("load fault at pc=%x\n", pc);
        assert(fault_pc == (uint32_t)GET32);
        load_fault_n++;
    } else {
        trace("store fault at pc=%x\n", pc);
        assert(fault_pc == (uint32_t)PUT32);
        store_fault_n++;
    }

    // disable the watchpoint.
    cp14_wcr0_disable();
    assert(!cp14_wcr0_is_enabled());

    // switch to where we should go.
    switchto(r);
}


void notmain(void) {
    enum { null = 0 };
    // initialize null.
    PUT32(null, 0xdeadbeef);
    trace("INITIAL VALUE: addr %x = %x\n", null, GET32(null));

    // install exception handlers: see <staff-full-except.c>
    full_except_install(0);
    full_except_set_data_abort(watchpt_fault);

    //uint32_t debug_id_macro = 0;

    // check that bits 0..31 of ~0 are 1.
    cp14_enable();
    assert(cp14_is_enabled());

    /* 
     * see: 
     *   - 13-47: how to set a simple watchpoint.
     *   - 13-17 for how to set bits in the <wcr0>
     */


/*
Read the WCR.
2. Clear the WCR[0] enable watchpoint bit in the read word and write it back to the WCR.
Now the watchpoint is disabled.
3. Write the DMVA to the WVR register.
4. Write to the WCR with its fields set as follows:
• WCR[20] enable linking bit cleared, to indicate that this watchpoint is not to be
linked
• WCR byte address select, load/store access, Secure access field, and supervisor
access fields as required
• WCR[0] enable watchpoint bit set.*/
    /**************************************************************
     * 1. store test with PUT32.
     */

    // set watchpoint.
    assert(!cp14_wcr0_is_enabled());
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
    cp14_wvr0_set(null);
    assert(cp14_wcr0_is_enabled());

    trace("set watchpoint for addr %p\n", null);

    trace("should see a store fault!\n");
    uint32_t val = 0xfaf0faf0;
    PUT32(null,val);

    if(!store_fault_n)
        panic("did not see a store fault\n");
    assert(!cp14_wcr0_is_enabled());

   // note, this should not fault.
    uint32_t x = GET32(null);
    if(x == val)
        trace("SUCCESS: correctly got a fault on addr=%x, val=%x\n", 
                                null, x);
    else
        panic("returned %x, expected %x\n", x, val);

    /**************************************************************
     * 1. load test with GET32.
     */

    // set up the fault again.
    trace("setting watchpoint for addr %p\n", null);
    cp14_wcr0_set(b);
    cp14_wcr0_enable();
    cp14_wvr0_set(null);
    assert(cp14_wcr0_is_enabled());

    trace("should see a load fault!\n");
    x = GET32(null);
    if(!load_fault_n)
        panic("did not see a load fault\n");
    if(x != val)
        panic("expected %x, got %x\n", val,x);
    trace("SUCCESS GET32 fault worked!\n");
}
