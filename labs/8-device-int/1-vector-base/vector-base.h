// once this works, move it to: 
//    <libpi/include/vector-base.h>
// and make sure it still works.
#ifndef __VECTOR_BASE_SET_H__
#define __VECTOR_BASE_SET_H__
#include "libc/bit-support.h"
#include "asm-helpers.h"
#include <stdint.h>


// define macros
cp_asm_set(VECBASE, p15, 0, c12, c0, 0);
cp_asm_get(VECBASE, p15, 0, c12, c0, 0);
//define cp_asm_set(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)

/*
 * vector base address register:
 *   arm1176.pdf:3-121 --- lets us control where the 
 *   exception jump table is!  makes it easy to switch
 *   tables and also make exceptions faster.
 *
 * defines: 
 *  - vector_base_set  
 *  - vector_base_get
 */

// return the current value vector base is set to.
static inline void *vector_base_get(void) {
   uint32_t reg = VECBASE_get();
//    uint32_t addr = bits_get(reg, 5, 31);
//    return (void*)(uintptr_t)addr;
return (void*)(uintptr_t)reg;

}

// check that not null and alignment is good.
static inline int vector_base_chk(void *vector_base) {
    if(!vector_base)
        return 0;
    if ((((uint32_t)(uintptr_t)vector_base) % 32 )!= 0) // why 256 bit aligned
        return 0;
    return 1;
}

// set vector base: must not have been set already.
static inline void vector_base_set(void *vec) {
    if(!vector_base_chk(vec))
        panic("illegal vector base %p\n", vec);

    void *v = vector_base_get();
    // if already set to the same vector, just return.
    if(v == vec)
        return;

    if(v) 
        panic("vector base register already set=%p\n", v);

    uint32_t newVal = (uint32_t) (uintptr_t)vec;
    // newVal = bits_set((uint32_t)(uintptr_t)v, 5, 31, newVal); // TODO:why not 5-31??
    VECBASE_set(newVal);
    // double check that what we set is what we have.
    v = vector_base_get();
    if(v != vec)
        panic("set vector=%p, but have %p\n", vec, v);
}

// set vector base to <vec> and return old value: could have
// been previously set (i.e., non-null).
static inline void *
vector_base_reset(void *vec) {
    void *old_vec = 0;
    if(!vector_base_chk(vec))
        panic("illegal vector base %p\n", vec);

    old_vec = vector_base_get();

    uint32_t newVal = (uint32_t) (uintptr_t)vec;
    //newVal = bits_set((uint32_t)old_vec, 5, 31, newVal);
    VECBASE_set(newVal);
    //double check that what we set is what we have.
    assert(vector_base_get() == vec);
    return (void*)old_vec;
}
#endif


// TODO: understand why this is faster?