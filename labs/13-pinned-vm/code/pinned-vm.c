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
cp_asm_set_fn(tlb_index, p15, 5, c15, c4, 2);
cp_asm_set_fn(tlb_va, p15, 5, c15, c5, 2);
cp_asm_set_fn(tlb_pa, p15, 5, c15, c6, 2);
cp_asm_set_fn(tlb_attr, p15, 5, c15, c7, 2);

cp_asm_get_fn(tlb_index, p15, 5, c15, c4, 2);
cp_asm_get_fn(tlb_va, p15, 5, c15, c5, 2);
cp_asm_get_fn(tlb_pa, p15, 5, c15, c6, 2);
cp_asm_get_fn(tlb_attr, p15, 5, c15, c7, 2);




uint32_t generate_mask_13(uint32_t start, uint32_t end) {
    if (start >= 32 || end > 32 || start >= end) {
        return 0; // Return 0 for invalid inputs
    }
    return ((1U << (end - start)) - 1) << start;
}

uint32_t read_modify_write_13(uint32_t originalValue, uint32_t bitNumberStart, uint32_t bitNumberEnd, uint32_t value) {
    uint32_t bitMask = generate_mask_13(bitNumberStart, bitNumberEnd);
    if (bitMask == 0) {
        return originalValue; // Return original value if mask is invalid
    }

    // Ensure value fits within the mask
    value &= (bitMask >> bitNumberStart);

    originalValue &= ~bitMask;  // Clear the bits in the range
    originalValue |= value << bitNumberStart; // Set new bits

    return originalValue;
}

char* to_binary(uint32_t number) {
    char* buffer = (char*)kmalloc(33 * sizeof(char)); // Allocate memory dynamically
    if (buffer == NULL) {
        return NULL; // Handle memory allocation failure
    }
    
    for (int i = 31; i >= 0; i--) {
        buffer[31 - i] = (number & (1U << i)) ? '1' : '0';
    }
    
    buffer[32] = '\0'; // Null-terminate the string
    return buffer;
}


static void *null_pt = 0;

// should we have a pinned version?
void domain_access_ctrl_set(uint32_t d) {
    staff_domain_access_ctrl_set(d);
}

// fill this in based on the <1-test-basic-tutorial.c>
// NOTE: 
//    you'll need to allocate an invalid page table
void pin_mmu_init(uint32_t domain_reg) {
    staff_pin_mmu_init(domain_reg);
    return;
}

// do a manual translation in tlb:
//   1. store result in <result>
//   2. return 1 if entry exists, 0 otherwise.
//
// NOTE: mmu must be on (confusing).
int tlb_contains_va(uint32_t *result, uint32_t va) {
    assert(mmu_is_enabled());

    // 3-79
    assert(bits_get(va, 0,2) == 0);
    return staff_tlb_contains_va(result, va);
}

// map <va>-><pa> at TLB index <idx> with attributes <e>
void pin_mmu_sec(unsigned idx,  
                uint32_t va, 
                uint32_t pa,
                pin_t e) {


    demand(idx < 8, lockdown index too large);
    // lower 20 bits should be 0.
    demand(bits_get(va, 0, 19) == 0, only handling 1MB sections);
    demand(bits_get(pa, 0, 19) == 0, only handling 1MB sections);

    debug("about to map %x->%x\n", va,pa);


    // delete this and do add your code below.
    //staff_pin_mmu_sec(idx, va, pa, e);
    // return;

    // these will hold the values you assign for the tlb entries.
    uint32_t x, va_ent, pa_ent, attr;
    // todo("assign these variables!\n");
    // tlb lockdown index
    x = tlb_index_get();
    trace("before idx is %s \n", to_binary(x));
    x = read_modify_write_13(x, 0, 3, idx);
    trace("after idx is %s \n", to_binary(x));
    tlb_index_set(idx);
    // tlb lockdown VA register
    // uint32_t i = 0;
    // i = read_modify_write_13(i, 9, 10, 1);
    // trace("try i %u \n", i);

    va_ent = tlb_va_get();
    trace("before va_ent is %s \n", to_binary(va_ent));
    va_ent = read_modify_write_13(va_ent, 12, 32, va); // TODO: set asid and secure mode
    va_ent = read_modify_write_13(va_ent, 9, 10, 1);
    va_ent = read_modify_write_13(va_ent, 0, 8, 0);
    trace("after va_ent is %s \n", to_binary(va_ent));
    tlb_va_set(va_ent);
    // // tlb lockdown PA register
    pa_ent = tlb_pa_get();
    trace("before pa_ent is %s \n", to_binary(pa_ent));
    pa_ent = read_modify_write_13(pa_ent, 12, 32, pa);
    pa_ent = read_modify_write_13(pa_ent, 6, 8, 3); // set size to 1MB sections
    // pa_ent = read_modify_write_13(pa_ent, 1, 3, 3); // AP set to 11
    // pa_ent = read_modify_write_13(pa_ent, 3, 4, 0); // APX set to 0
    pa_ent = read_modify_write_13(pa_ent, 0, 1, 1); // entry is valid
    pa_ent |= perm_ro_priv;
    tlb_pa_set(pa_ent);
    trace("after pa_ent is %s \n", to_binary(va_ent));
    // // tlb lockdown attributes register
    attr = tlb_attr_get();
    trace("before attr is %s \n", to_binary(attr));
    attr = read_modify_write_13(attr, 7, 10, DOM_manager); // set domain to 0
    attr |= MEM_device; // set tex, c, b to shareable
    tlb_attr_set(attr);
    trace("after attr is %s \n", to_binary(attr));

    // put your code here.
    unimplemented();

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
int pin_exists(uint32_t va, int verbose_p) {
    if(!mmu_is_enabled())
        panic("XXX: i think we can only check existence w/ mmu enabled\n");

    uint32_t r;
    if(tlb_contains_va(&r, va)) {
        assert(va == r);
        return 1;
    } else {
        if(verbose_p) 
            output("TLB should have %x: returned %x [reason=%b]\n", 
                va, r, bits_get(r,1,6));
        return 0;
    }
}

// look in test <1-test-basic.c> to see what to do.
// need to set the <asid> before turning VM on and 
// to switch processes.
void pin_set_context(uint32_t asid) {
    // put these back
    // demand(asid > 0 && asid < 64, invalid asid);
    // demand(null_pt, must setup null_pt --- look at tests);

    staff_pin_set_context(asid);
}

void pin_clear(unsigned idx)  {
    staff_pin_clear(idx);
}


void staff_lockdown_print_entry(unsigned idx);

void lockdown_print_entry(unsigned idx) {

    staff_lockdown_print_entry(idx);
}

void lockdown_print_entries(const char *msg) {
    trace("-----  <%s> ----- \n", msg);
    trace("  pinned TLB lockdown entries:\n");
    for(int i = 0; i < 8; i++)
        lockdown_print_entry(i);
    trace("----- ---------------------------------- \n");
}