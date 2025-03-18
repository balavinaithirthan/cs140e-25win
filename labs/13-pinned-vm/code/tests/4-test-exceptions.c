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

#define MB(x) ((x) * 1024 * 1024)

// These are the default segments (segment = one MB)
// that need to be mapped for our binaries so far
// this quarter.
//
// these will hold for all our tests today.
//
// if we just map these segments we will get faults
// for stray pointer read/writes outside of this region.
enum
{
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

enum
{
    dom_kern = 1,
    dom_heap = 2
};

// used to store the illegal address we will write.
static volatile uint32_t illegal_addr;

// macros to generate inline assembly routines.  recall
// we've used these for the debug lab, etc.
#include "asm-helpers.h"

// b4-44: get the fault address from a data abort.
cp_asm_get(cp15_fault_addr, p15, 0, c6, c0, 0) cp_asm_get(cp15_dfsr, p15, 0, c5, c0, 0);
// a trivial data abort handler that checks that we
// faulted on the address that we expected.
//   - note: bad form that we don't actually check
//     that its a data abort.

static void data_abort_handler(regs_t *r)
{
    uint32_t fault_addr;

#if 0
    // b4-44
    // alternatively you can use the inline assembly raw.
    // can be harder to debug.
    asm volatile("MRC p15, 0, %0, c6, c0, 0" : "=r" (fault_addr));
#else
    fault_addr = cp15_fault_addr_get();
    uint32_t dfsr = cp15_dfsr_get();
    uint32_t fs = (dfsr & 0xF) | ((dfsr >> 6) & 0x10);
    // uint32_t reason = (bit_get(dfsr, 10) << 10) | bits_get(dfsr, 0, 3);

#endif
    for (int i = 0; i < 17; i++)
    {
        output("r[%d] = %x\n", i, r->regs[i]);
    }
    switch (fs)
    {
        case 0b00001:
            printk("Alignment Fault: Misaligned memory access.\n");
            break;
        case 0b00011:
            printk("Debug Event: Debug exception occurred.\n");
            break;
        case 0b00101:
            printk("Translation Fault (Section): MMU failed to translate (section).\n");
            break;
        case 0b00111:
            printk("Translation Fault (Page): MMU failed to translate (page).\n");
            break;
        case 0b01001:
            printk("Domain Fault (Section): Domain permissions denied access (section).\n");
            break;
        case 0b01011:
            printk("Domain Fault (Page): Domain permissions denied access (page).\n");
            break;
        case 0b01101:
            printk(
                "Permission Fault (Section): Access denied by page table permissions (section).\n");
            break;
        case 0b01111:
            printk("Permission Fault (Page): Access denied by page table permissions (page).\n");
            break;
        case 0b10110:
            printk("External Abort (Section): Hardware bus error (section).\n");
            break;
        case 0b10111:
            printk("External Abort (Page): Hardware bus error (page).\n");
            break;
        default:
            printk("Unknown Fault Status: 0x%x\n", fs);
            break;
    }
    // trace("the pc of the fault is fault addr %x, the pc is %x and the reason is %d \n",
    // fault_addr,
    //   r->regs[15], reason);
    assert(r->regs[0] == fault_addr);

    uint32_t combined_domain = (DOM_client << dom_kern * 2) | (DOM_client << dom_heap * 2);
    staff_domain_access_ctrl_set(combined_domain);

    asm volatile(
        "mov r0, %0\n"  // Move regs[15] + 4 into r0
        "bx r0\n"       // Branch to the address in r0
        :
        : "r"(r->regs[14] + 4)
        : "r0");
}

void notmain(void)
{
    kmalloc_init_set_start((void *)SEG_HEAP, MB(1));
    assert((uint32_t)__prog_end__ < SEG_CODE + MB(1));
    assert((uint32_t)__code_start__ >= SEG_CODE);
    full_except_install(0);
    full_except_set_data_abort(data_abort_handler);
    void *null_pt = kmalloc_aligned(4096 * 4, 1 << 14);
    assert((uint32_t)null_pt % (1 << 14) == 0);
    assert(!mmu_is_enabled());
    enum
    {
        no_user = perm_rw_priv
    };
    pin_t dev = pin_mk_global(dom_kern, no_user, MEM_device);
    pin_t kern = pin_mk_global(dom_kern, no_user, MEM_uncached);
    pin_t heapKern = pin_mk_global(dom_heap, no_user, MEM_uncached);
    staff_mmu_init();
    pin_mmu_sec(0, SEG_CODE, SEG_CODE, kern);
    pin_mmu_sec(1, SEG_HEAP, SEG_HEAP, heapKern);
    pin_mmu_sec(2, SEG_STACK, SEG_STACK, kern);
    pin_mmu_sec(3, SEG_INT_STACK, SEG_INT_STACK, kern);
    pin_mmu_sec(4, SEG_BCM_0, SEG_BCM_0, dev);
    pin_mmu_sec(5, SEG_BCM_1, SEG_BCM_1, dev);
    pin_mmu_sec(6, SEG_BCM_2, SEG_BCM_2, dev);
    uint32_t combined_domain = (DOM_client << dom_kern * 2) | (DOM_client << dom_heap * 2);
    staff_domain_access_ctrl_set(combined_domain);

    enum
    {
        ASID = 1,
        PID = 128
    };
    staff_mmu_set_ctx(PID, ASID, null_pt);

    trace("about to enable\n");
    for (int i = 0; i < 10; i++)
    {
        assert(!staff_mmu_is_enabled());
        // mmu.h: you'll implement next vm lab.
        staff_mmu_enable();

        // this uses: stack, code, data, BCM.
        if (staff_mmu_is_enabled())
            trace("MMU ON: hello from virtual memory!  cnt=%d\n", i);
        else
            panic("MMU is not on?\n");

        // mmu.h: you'll implement next vm lab.
        staff_mmu_disable();
        assert(!staff_mmu_is_enabled());
        trace("MMU is off!\n");
    }

    // this <PUT32> should "work" since vm is off.
    assert(!staff_mmu_is_enabled());
    PUT32(SEG_HEAP, 0xdeadbeef);
    trace("we wrote without vm: got %x\n", GET32(SEG_HEAP));
    assert(GET32(SEG_HEAP) == 0xdeadbeef);

    // this <PUT32> should "work" since vm is on and domain is chill.
    staff_mmu_enable();
    assert(staff_mmu_is_enabled());
    PUT32(SEG_HEAP, 0xdeadbeef);
    assert(GET32(SEG_HEAP) == 0xdeadbeef);

    ////////
    // remove domain now: GET32
    uint32_t single_domain = (DOM_client << dom_kern * 2);
    staff_domain_access_ctrl_set(single_domain);
    assert(staff_mmu_is_enabled());
    trace("Get32 about to\n");
    GET32(SEG_HEAP);
    trace("back to main!\n");
    // remove domain: PUT32
    single_domain = (DOM_client << dom_kern * 2);
    staff_domain_access_ctrl_set(single_domain);
    assert(staff_mmu_is_enabled());
    trace("Put32 about to\n");
    PUT32(SEG_HEAP, 0xdeadbeef);
    trace("back to main!\n");
    // Jump test
    single_domain = (DOM_client << dom_kern * 2);
    staff_domain_access_ctrl_set(single_domain);
    assert(staff_mmu_is_enabled());
    uint32_t *heap_mem = (uint32_t *)SEG_HEAP;
    *heap_mem = 0xe12fff1e;
    trace("jump about to\n");
    BRANCHTO((uint32_t)heap_mem);
    trace("done jumping \n");
}
