// complete working example of trivial vm:
//  1. identity map the kernel (code, heap, kernel stack, 
//     exception stack, device memory) using 1MB segments.
//  2. turn VM on/off making sure we can print.
//  3. with VM on, write to an unmapped address and make sure we 
//     get a fault.
//
//  the routines that start with "pin_" are what you'll build today,
//  the ones with with "staff_mmu_" are low level hardware 
//  routines that you'll build next VM lab.
//
//  there are some page numbers: if they start with a "B" they
//  are from the armv6 pdf, if with a number, from arm1176 pdf
#include "rpi.h"
#include "pinned-vm.h"
#include "full-except.h"
#include "memmap.h"

#define MB(x) ((x)*1024*1024)

// These are the default segments (segment = one MB)
// that need to be mapped for our binaries so far
// this quarter. 
//
// these will hold for all our tests today.
//
// if we just map these segments we will get faults
// for stray pointer read/writes outside of this region.
enum {
    // code starts at 0x8000, so map the first MB
    //
    // if you look in <libpi/memmap> you can see
    // that all the data is there as well, and we have
    // small binaries, so this will cover data as well.
    //
    // NOTE: obviously it would be better to not have 0 (null) 
    // mapped, but our code starts at 0x8000 and we are using
    // 1mb sections (which require 1MB alignment) so we don't
    // have a choice unless we do some modifications.  
    //
    // you can fix this problem as an extension: very useful!
    SEG_CODE = MB(0),

    // as with previous labs, we initialize 
    // our kernel heap to start at the first 
    // MB. it's 1MB, so fits in a segment. 
    SEG_HEAP = MB(1),

    // if you look in <staff-start.S>, our default
    // stack is at STACK_ADDR, so subtract 1MB to get
    // the stack start.
    SEG_STACK = STACK_ADDR - MB(1),

    // the interrupt stack that we've used all class.
    // (e.g., you can see it in the <full-except-asm.S>)
    // subtract 1MB to get its start
    SEG_INT_STACK = INT_STACK_ADDR - MB(1),

    // the base of the BCM device memory (for GPIO
    // UART, etc).  Three contiguous MB cover it.
    SEG_BCM_0 = 0x20000000,
    SEG_BCM_1 = SEG_BCM_0 + MB(1),
    SEG_BCM_2 = SEG_BCM_0 + MB(2),

    // we guarantee this (2MB) is an 
    // unmapped address
    SEG_ILLEGAL = MB(2),
};

// used to store the illegal address we will write.
static volatile uint32_t illegal_addr;

// macros to generate inline assembly routines.  recall
// we've used these for the debug lab, etc.
#include "asm-helpers.h"

// b4-44: get the fault address from a data abort.
cp_asm_get(cp15_fault_addr, p15, 0, c6, c0, 0)
cp_asm_get(cp15_dfsr, p15, 0, c5, c0, 0)
// a trivial data abort handler that checks that we 
// faulted on the address that we expected.
//   - note: bad form that we don't actually check 
//     that its a data abort.
static void data_abort_handler(regs_t *r) {
    uint32_t fault_addr;

#if 0
    // b4-44
    // alternatively you can use the inline assembly raw.
    // can be harder to debug.
    asm volatile("MRC p15, 0, %0, c6, c0, 0" : "=r" (fault_addr));
#else
    fault_addr = cp15_fault_addr_get();
    uint32_t dfsr = cp15_dfsr_get();
    uint32_t reason = (bit_get(dfsr, 10) << 10) | bits_get(dfsr, 0, 3);

#endif
    for (int i = 0; i < 17; i++) {
        output("r[%d] = %x\n", i, r->regs[i]);
    }
    trace("the pc of the fault is fault addr %x, the pc is %x and the reason is %d \n", fault_addr, r->regs[15], reason);
    assert(r->regs[3] == fault_addr);

    // make sure we faulted on the address that should be accessed.

    if(fault_addr != illegal_addr)
        panic("illegal fault!  expected %x, got %x\n",
            illegal_addr, fault_addr);
    else
        trace("SUCCESS!: got a fault on address=%x\n", fault_addr);
    
    enum { dom_kern = 1 };
    staff_domain_access_ctrl_set(DOM_client << dom_kern*2);
    asm volatile (
        "mov r0, %0\n"  // Move fault_address into register r0
        "bx r0\n"        // Branch to the address stored in r0
        :
        : "r" (r->regs[15] + 4)
        : "r0"
    );

}

void notmain(void) { 

    kmalloc_init_set_start((void*)SEG_HEAP, MB(1));
    assert((uint32_t)__prog_end__ < SEG_CODE + MB(1));
    assert((uint32_t)__code_start__ >= SEG_CODE);
    full_except_install(0);
    full_except_set_data_abort(data_abort_handler);
    void *null_pt = kmalloc_aligned(4096*4, 1<<14);
    assert((uint32_t)null_pt % (1<<14) == 0);
    assert(!mmu_is_enabled());
    enum { no_user = perm_rw_priv };
    enum { dom_kern = 1 };
    pin_t dev  = pin_mk_global(dom_kern, no_user, MEM_device);
    pin_t kern = pin_mk_global(dom_kern, no_user, MEM_uncached);
    staff_mmu_init();
    pin_mmu_sec(0, SEG_CODE, SEG_CODE, kern);
    pin_mmu_sec(1, SEG_HEAP, SEG_HEAP, kern); 
    pin_mmu_sec(2, SEG_STACK, SEG_STACK, kern); 
    pin_mmu_sec(3, SEG_INT_STACK, SEG_INT_STACK, kern); 
    pin_mmu_sec(4, SEG_BCM_0, SEG_BCM_0, dev); 
    pin_mmu_sec(5, SEG_BCM_1, SEG_BCM_1, dev); 
    pin_mmu_sec(6, SEG_BCM_2, SEG_BCM_2, dev); 
    // TODO: why do I have to set everything
    staff_domain_access_ctrl_set(DOM_client << dom_kern*2); 

    enum { ASID = 1, PID = 128 };
    staff_mmu_set_ctx(PID, ASID, null_pt);

    trace("about to enable\n");
    // for(int i = 0; i < 10; i++) {
        assert(!staff_mmu_is_enabled());
        staff_mmu_enable();
        if(staff_mmu_is_enabled())
            trace("MMU ON: hello from virtual memory!  cnt= 1\n");
        else
            panic("MMU is not on?\n");
        // staff_mmu_disable();
        // assert(!staff_mmu_is_enabled());
        // trace("MMU is off!\n");
    // }

    // ******************************************************
    // 5. setup exception handling and make sure we get a fault.
    // 
    //    you should make a copy of this test case, and 

    // we write to an address we know is unmapped so we get
    // a fault.
    illegal_addr = SEG_ILLEGAL;

    // this <PUT32> should "work" since vm is off.
    // assert(!staff_mmu_is_enabled());
    PUT32(illegal_addr, 0xdeadbeef);
    unsigned int pc_value;

    asm volatile (
        "mov %0, pc\n"  // Move the value of the program counter into pc_value
        : "=r" (pc_value) // Output operand
    );
    trace("we wrote without vm: got %x\n", GET32(illegal_addr));
    panic("hi");
    // assert(GET32(illegal_addr) == 0xdeadbeef);

    // now try with MMU on: it's a good exercise to make a copy
    // of this test case and change it so you repeatedly fault.
    staff_mmu_enable();
    assert(staff_mmu_is_enabled());
    output("about to write to <%x>\n", illegal_addr);
    // this <PUT32> should cause a data_abort.
    PUT32(illegal_addr, 0xdeadbeef);
    panic("should not reach here\n");
}
