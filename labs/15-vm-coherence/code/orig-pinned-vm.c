// put your code here.
//
#include "rpi.h"
#include "libc/bit-support.h"

// has useful enums and helpers.
#include "vector-base.h"
#include "pinned-vm.h"
#include "mmu.h"
#include "procmap.h"

// generate the _get and _set methods.
// (see asm-helpers.h for the cp_asm macro
// definition)
// arm1176.pdf: 3-149

uint32_t generate_mask_13(uint32_t start, uint32_t end)
{
    if (start >= 32 || end > 32 || start >= end)
    {
        return 0;  // Return 0 for invalid inputs
    }
    return ((1U << (end - start)) - 1) << start;
}

uint32_t read_modify_write_13(uint32_t originalValue, uint32_t bitNumberStart,
                              uint32_t bitNumberEnd, uint32_t value)
{
    uint32_t bitMask = generate_mask_13(bitNumberStart, bitNumberEnd);
    if (bitMask == 0)
    {
        return originalValue;  // Return original value if mask is invalid
    }

    // Ensure value fits within the mask
    value &= (bitMask >> bitNumberStart);

    originalValue &= ~bitMask;                 // Clear the bits in the range
    originalValue |= value << bitNumberStart;  // Set new bits

    return originalValue;
}

cp_asm_set_fn(tlb_index, p15, 5, c15, c4, 2);
cp_asm_set_fn(tlb_va, p15, 5, c15, c5, 2);
cp_asm_set_fn(tlb_pa, p15, 5, c15, c6, 2);
cp_asm_set_fn(tlb_attr, p15, 5, c15, c7, 2);
cp_asm_set_fn(domain_access_control, p15, 0, c3, c0, 0);

cp_asm_get_fn(tlb_index, p15, 5, c15, c4, 2);
cp_asm_get_fn(tlb_va, p15, 5, c15, c5, 2);
cp_asm_get_fn(tlb_pa, p15, 5, c15, c6, 2);
cp_asm_get_fn(tlb_attr, p15, 5, c15, c7, 2);
cp_asm_get_fn(domain_access_control, p15, 0, c3, c0, 0);

cp_asm_get_fn(va_to_pa, p15, 0, c7, c8, 0);
cp_asm_set_fn(va_to_pa, p15, 0, c7, c8, 0);

cp_asm_get_fn(pa, p15, 0, c7, c4, 0);
cp_asm_set_fn(pa, p15, 0, c7, c4, 0);

static void *null_pt = 0;

// fill this in based on the <1-test-basic-tutorial.c>
// NOTE:
//    you'll need to allocate an invalid page table
void pin_mmu_init(uint32_t domain_reg)
{
    mmu_init();
    null_pt = (void *)kmalloc_aligned(4096 * 4, 1 << 14);
    assert((uint32_t)null_pt % (1 << 14) == 0);
    domain_access_ctrl_set(domain_reg);
}

// do a manual translation in tlb:
//   1. store result in <result>
//   2. return 1 if entry exists, 0 otherwise.
//
// NOTE: mmu must be on (confusing).
int tlb_contains_va(uint32_t *result, uint32_t va)
{
    assert(mmu_is_enabled());
    assert(bits_get(va, 0, 2) == 0);  // Ensure va is properly aligned

    va_to_pa_set(va);
    uint32_t pa_conv = pa_get();

    *result = pa_conv;
    *result = (bits_get(pa_conv, 10, 31) << 10) | bits_get(va, 0, 9);
    uint32_t success = bits_get(pa_conv, 0, 0);  // Check success bit
    if (success == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// map <va>-><pa> at TLB index <idx> with attributes <e>
void pin_mmu_sec(unsigned idx, uint32_t va, uint32_t pa, pin_t e)
{
    demand(idx < 8, lockdown index too large);
    // lower 20 bits should be 0.
    demand(bits_get(va, 0, 19) == 0, only handling 1MB sections);
    demand(bits_get(pa, 0, 19) == 0, only handling 1MB sections);

    debug("about to map %x->%x\n", va, pa);

    // these will hold the values you assign for the tlb entries.
    uint32_t x, va_ent, pa_ent, attr;
    // todo("assign these variables!\n");
    // tlb lockdown index
    x = tlb_index_get();
    // trace("before idx is %s \n", to_binary(x));
    x = read_modify_write_13(x, 0, 3, idx);
    // trace("after idx is %s \n", to_binary(x));
    tlb_index_set(idx);
    // tlb lockdown VA register
    // uint32_t i = 0;
    // i = read_modify_write_13(i, 9, 10, 1);
    // trace("try i %u \n", i);

    va_ent = va;  // so this is the entry number and the last 12 bytes
    // trace("before va_ent is %s \n", to_binary(va_ent));
    // va_ent = read_modify_write_13(va_ent, 12, 32, va); // TODO: set asid and
    // secure mode
    va_ent = read_modify_write_13(va_ent, 9, 10, e.G);
    va_ent = read_modify_write_13(va_ent, 0, 8, e.asid);
    // trace("after va_ent is %s \n", to_binary(va_ent));
    tlb_va_set(va_ent);
    // // tlb lockdown PA register
    pa_ent = pa;
    // trace("before pa_ent is %s \n", to_binary(pa_ent));
    // pa_ent = read_modify_write_13(pa_ent, 12, 32, pa);
    pa_ent = read_modify_write_13(pa_ent, 6, 8,
                                  e.pagesize);  // set size to 1MB sections
    uint32_t APX = e.AP_perm >> 2;
    pa_ent = read_modify_write_13(pa_ent, 3, 4, APX);  // AP bit
    uint32_t AP = e.AP_perm & 3;
    pa_ent = read_modify_write_13(pa_ent, 1, 3, AP);  // APX set to 0
    pa_ent = read_modify_write_13(pa_ent, 0, 1, 1);   // entry is valid
    tlb_pa_set(pa_ent);
    // trace("after pa_ent is %s \n", to_binary(va_ent));
    // // tlb lockdown attributes register
    attr = 0;
    // trace("before attr is %s \n", to_binary(attr));
    attr = read_modify_write_13(attr, 7, 11, e.dom);  // set domain to 0
    attr = read_modify_write_13(attr, 1, 6, e.mem_attr);
    tlb_attr_set(attr);
    // trace("after attr is %s \n", to_binary(attr));

    // put your code here.
    // unimplemented();

#if 0
if((x = lockdown_va_get()) != va_ent)
panic("lockdown va: expected %x, have %x\n", va_ent,x);
if((x = lockdown_pa_get()) != pa_ent)
panic("lockdown pa: expected %x, have %x\n", pa_ent,x);
if((x = lockdown_attr_get()) != attr)
panic("lockdown attr: expected %x, have %x\n", attr,x);
#endif
}

// check that <va> is pinned.
int pin_exists(uint32_t va, int verbose_p)
{
    if (!mmu_is_enabled()) panic("XXX: i think we can only check existence w/ mmu enabled\n");

    uint32_t r;
    if (tlb_contains_va(&r, va))
    {
        assert(va == r);
        return 1;
    }
    else
    {
        if (verbose_p)
            output("TLB should have %x: returned %x [reason=%b]\n", va, r, bits_get(r, 1, 6));
        return 0;
    }
}

// look in test <1-test-basic.c> to see what to do.
// need to set the <asid> before turning VM on and
// to switch processes.
void pin_set_context(uint32_t asid)
{
    // put these back
    demand(asid > 0 && asid < 64, invalid asid);
    demand(null_pt, must setup null_pt-- - look at tests);
    mmu_set_ctx(128, asid, null_pt);
}

void pin_clear(unsigned idx)
{
    tlb_index_set(idx);
    uint32_t va_ent = 0;  // so this is the entry number and the last 12 bytes
    tlb_va_set(va_ent);
    assert(tlb_va_get() == 0);
    uint32_t pa_ent = 0;
    tlb_pa_set(pa_ent);
    assert(tlb_pa_get() == 0);
    uint32_t attr = 0;
    tlb_attr_set(attr);
    assert(tlb_attr_get() == 0);
}

void lockdown_print_entry(unsigned idx)
{
    trace("   idx=%d\n", idx);
    tlb_index_set(idx);
    uint32_t va_ent = tlb_va_get();
    uint32_t pa_ent = tlb_pa_get();
    unsigned v = bit_get(pa_ent, 0);

    if (!v)
    {
        trace("     [invalid entry %d]\n", idx);
        return;
    }

    // // 3-149
    uint32_t G = bits_get(va_ent, 9, 9);
    uint32_t asid = bits_get(va_ent, 0, 7);
    uint32_t va = bits_get(va_ent, 12, 31);
    trace("     va_ent=%x: va=%x|G=%d|ASID=%d\n", va_ent, va, G, asid);

    // // 3-150
    // ...fill in the needed vars...
    uint32_t size = bits_get(pa_ent, 6, 7);
    uint32_t apx = bits_get(pa_ent, 1, 3);
    // uint32_t ap = bits_get(pa_ent, 1, 2);
    uint32_t nsa = bits_get(pa_ent, 9, 9);
    uint32_t nstid = bits_get(pa_ent, 8, 8);
    uint32_t pa = bits_get(pa_ent, 12, 31);
    trace("     pa_ent=%x: pa=%x|nsa=%d|nstid=%d|size=%b|apx=%b|v=%d\n", pa_ent, pa, nsa, nstid,
          size, apx, v);

    // // 3-151
    // ...fill in the needed vars...
    uint32_t attr = tlb_attr_get();
    uint32_t dom = bits_get(attr, 7, 10);
    uint32_t xn = bit_get(attr, 6);
    uint32_t tex = bits_get(attr, 3, 5);
    uint32_t C = bit_get(attr, 2);
    uint32_t B = bit_get(attr, 1);
    trace("     attr=%x: dom=%d|xn=%d|tex=%b|C=%d|B=%d\n", attr, dom, xn, tex, C, B);
}

void lockdown_print_entries(const char *msg)
{
    trace("-----  <%s> ----- \n", msg);
    trace("  pinned TLB lockdown entries:\n");
    for (int i = 0; i < 8; i++) lockdown_print_entry(i);
    trace("----- ---------------------------------- \n");
}

/*
Big Idea:

We implement a set of functions that users can use in order to work with the
lockdown registers ie pinned TLB entries. Lab 13: pinned vm We implement
pin_mmu_sec which pins a virtual to physical address mapping



Lab 15:
We need to allow the user to initialize the mmu
1. mem reset
2. initialize access to domain region
3. Set up an empty page table


*/
