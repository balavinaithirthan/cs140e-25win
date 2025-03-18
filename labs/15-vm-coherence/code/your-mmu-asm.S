@ Must carefully read B2 of the armv6 manual.  You cannot rely on "well it
@ worked on my test case": mistakes cause stale data, which may or may not
@ show up in your test case (likely not) despite being wildly broken.
@
@ Note: Rd is allegedly a read-only parameter to these instructions, but
@ the manual says SBZ (should be zero).  I think (and the linux source seems
@ to think) that this means we have to explicitly set it to 0.  Not setting
@ Rd=0 is an easy mistake to make.  Perhaps is worth building in (Linux does
@ not, but) --- the cost of an extra mov is negligible given our other 
@ overheads.
@
@ Alot of people put these instructions in inline assembly, wrapped up by 
@ function calls (e.g., cs107e's MMU code).  This practice is dangerous. 
@ For example, if you are flushing caches, you cannot be sure that the 
@ function return, etc does not then reload them, esp w.r.t. the BTB.  I 
@ think the only safe approach is to put directly in assembly so you are 
@ guaranteed no branch, indirect jump, load/store happens in between.
@
@ A lot of MMU code out there appears pretty broken b/c they don't do enough
@ flushing operations and/or rely on gcc code generation to not do the above.

#include "rpi-asm.h"
#include "armv6-coprocessor-asm.h"


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ many page numbers are in 
@    armv6-coprocessor-asm.h
@ and the other armv6-*.h files

// just read the cache type: use C code to figure out if it is unified
// b6-13, b6-14
MK_FN(get_cache_type)
    asm_not_implemented()
    bx lr

// void reset(void)
// clear the dcache, icache, tlbs
/*
    1: STR rx  ; instruction location
    2: clean data cache by MVA ; instruction location
    3: DSB  ; ensures visibility of (2)
    4: invalidate icache by MVA
    5: Invalidate BTB
    6: DSB; ensure (4) completes
    7: Prefetch flush
    note: doing nothing with actually mapping rn, mapping stays the same
*/
MK_FN(mmu_reset)
    push {lr}
    INV_DCACHE(r0)
    DSB(r1)
    INV_ICACHE(r2)
    FLUSH_BTB(r3)
    INV_TLB(r0)
    DSB(r1)
    PREFETCH_FLUSH(r2)
    pop {lr}
    bx lr

// void cp15_domain_ctrl_wr(uint32_t dom_reg);
//  need to do cp15 sync: flush_btb, dsb, prefetch flush
MK_FN(cp15_domain_ctrl_wr)
    MCR p15, 0, r0, c1, c0, 0
    PREFETCH_FLUSH(r2)
    bx lr

// void cp15_set_procid_ttbr0(uint32_t pid, uint32_t asid, void *pt);
// sequence from b2-25

/*
Goal: given a proccessor and address id, we should be able to set TLB entries to have them
- we have the page table pointer as "null_pt"
 */
MK_FN(cp15_set_procid_ttbr0)
    MOV R3, #0
    lsl r1, r1, #14 @ shift left by 14
    TTBR0_SET(r1)  @ write to register
    TTBR_BASE_CTRL_WR(R3)
    DSB(r2)
    PREFETCH_FLUSH(r2)
    ASID_SET(R3)
    PREFETCH_FLUSH(r2)
    TTBR1_SET(R3)
    DSB(r1)
    PREFETCH_FLUSH(r2)
    ASID_SET(r0)
    push {r0}
    pop {r0}

    CLEAN_INV_DCACHE(r1)
    INV_ICACHE(r1)
    FLUSH_BTB(r1)
    DSB(r1)
    PREFETCH_FLUSH(r1)

    bx lr

// void mmu_disable_set_asm(cp15_ctrl_reg1_t c);
MK_FN(mmu_disable_set_asm)
    CLEAN_INV_DCACHE(r1)
    INV_ICACHE(r1)
    FLUSH_BTB(r1)
    DSB(r1)
    PREFETCH_FLUSH(r1)
    @ note: this piece of code is mapped into the same place in both
    @ MMU-enabled and MMU-disabled so BTB and Prefetch entries work identically
    @ to both MMU-enable and disable.   Without this requirement I believe is
    @ very hard to get right (perhaps impossible?) since we cannot flush
    @ the BTB and prefetch and switch MMU on/off atomically.  thus, would
    @ flush, but instructions could get fetched before we could disable,
    @ defeating the purpose.
    CONTROL_REG1_WR(r0)
    @ asm_not_implemented()
    bx lr

@ void mmu_enable_set_asm(cp15_ctrl_reg1_t c);
MK_FN(mmu_enable_set_asm)
    @ asm_not_implemented()
    TLB_CONFIG_RD(r1)
    FLUSH_BTB(r1)
    DSB(r1)
    PREFETCH_FLUSH(r1)
    CONTROL_REG1_WR(r0)
    bx lr

MK_FN(mmu_sync_pte_mods) 
    push {lr}
    CLEAN_INV_DCACHE(r0)
    DSB(r1)
    INV_ICACHE(r2)
    FLUSH_BTB(r3)
    INV_TLB(r0)
    DSB(r1)
    PREFETCH_FLUSH(r2)
    pop {lr}
    bx lr


@ cp15_ctrl_reg1_t cp15_ctrl_reg1_rd(void)
MK_FN(_cp15_ctrl_reg1_rd)
    @ asm_not_implemented()
    MRC p15, 0, r0, c1, c0, 0
    PREFETCH_FLUSH(r2)
    bx lr

MK_FN(_cp15_ctrl_reg1_wr)
    MCR p15, 0, r0, c1, c0, 0
    PREFETCH_FLUSH(r2)
    bx lr




