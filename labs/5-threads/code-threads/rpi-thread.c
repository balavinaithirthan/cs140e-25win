// engler, cs140e: starter code for trivial threads package.
#include "rpi.h"
#include "rpi-thread.h"

//***********************************************************
// debugging code: tracing and redzone checking.

// if you want to turn off tracing, just 
// change to "if 0"
#if 1
#   define th_trace(args...) trace(args)
#else
#   define th_trace(args...) do { } while(0)
#endif

// if you want to turn off redzone checking,
// change to "if 0"
#if 1
#   include "redzone.h"
#   define RZ_CHECK() redzone_check(0)
#else
#   define RZ_CHECK() do { } while(0)
#endif


/******************************************************************
 * datastructures used by the thread code.
 *
 * you don't have to modify this.
 */

#define E rpi_thread_t
#include "libc/Q.h"

// currently only have a single run queue and a free queue.
// the run queue is FIFO.
static Q_t runq, freeq;
static rpi_thread_t *cur_thread;        // current running thread.
static rpi_thread_t *scheduler_thread;  // first scheduler thread.

// monotonically increasing thread id: won't wrap before reboot :)
static unsigned tid = 1;

/******************************************************************
 * simplistic pool of thread blocks: used to make alloc/free 
 * faster (plus, our kmalloc doesn't have free (other than reboot).
 *
 * you don't have to modify this.
 */

// total number of thread blocks we have allocated.
static unsigned nalloced = 0;

// keep a cache of freed thread blocks.  call kmalloc if run out.
static rpi_thread_t *th_alloc(void) {
    RZ_CHECK();
    rpi_thread_t *t = Q_pop(&freeq);

    if(!t) {
        t = kmalloc_aligned(sizeof *t, 8);
        nalloced++;
    }
#   define is_aligned(_p,_n) (((unsigned)(_p))%(_n) == 0)
    demand(is_aligned(&t->stack[0],8), stack must be 8-byte aligned!);
    t->tid = tid++;
    // th_trace("tid being increased \n");
    return t;
}

static void th_free(rpi_thread_t *th) {
    RZ_CHECK();
    // push on the front in case helps with caching.
    Q_push(&freeq, th);
}


/*****************************************************************
 * implement the code below.
 */

// stack offsets we expect.
//  - see <code-asm-checks/5-write-regs.c>
enum {
    R4_OFFSET = 0,
    R5_OFFSET,
    R6_OFFSET,
    R7_OFFSET,
    R8_OFFSET,
    R9_OFFSET,
    R10_OFFSET,
    R11_OFFSET,
    R14_OFFSET = 8,
    LR_OFFSET = 8
};

// enum {
//     R4_OFFSET = 8,
//     R5_OFFSET = 7,
//     R6_OFFSET = 6,
//     R7_OFFSET = 5,
//     R8_OFFSET = 4,
//     R9_OFFSET = 3,
//     R10_OFFSET = 2,
//     R11_OFFSET = 1,
//     R14_OFFSET = 0,
//     LR_OFFSET = 0
// };

// return pointer to the current thread.  
rpi_thread_t *rpi_cur_thread(void) {
    //printk("current thread is %d \n", cur_thread->tid);
    assert(cur_thread);
    RZ_CHECK();
    return cur_thread;
}

// create a new thread.

rpi_thread_t *rpi_fork(void (*code)(void *arg), void *arg) {
    RZ_CHECK();
    rpi_thread_t *t = th_alloc();

    // write this so that it calls code,arg.
    void rpi_init_trampoline(void);

    /*
     * must do the "brain surgery" (k.thompson) to set up the stack
     * so that when we context switch into it, the code will be
     * able to call code(arg).
     *
     *  1. write the stack pointer with the right value.
     *  2. store arg and code into two of the saved registers.
     *  3. store the address of rpi_init_trampoline into the lr
     *     position so context switching will jump there.
     *
     * - see <code-asm-checks/5-write-regs.c> for how to 
     *   coordinate offsets b/n asm and C code.
     */
    t->saved_sp = t->stack + THREAD_MAXSTACK - 9;

    t->saved_sp[LR_OFFSET] = (uintptr_t)&rpi_init_trampoline;
    t->saved_sp[R4_OFFSET] = (uintptr_t) code;
    t->saved_sp[R5_OFFSET] = (uintptr_t)arg;

    // printk("stack pointer: %p\n", t->saved_sp);
    // assert(t->stack <= t->saved_sp && t->saved_sp <= &t->stack[THREAD_MAXSTACK]);
    /////
    t->fn = code;
    t->arg = arg;
    // should check that <t->saved_sp> points within the 
    // thread stack.
    th_trace("rpi_fork: tid=%d, code=[%p], arg=[%x], saved_sp=[%p]\n",
            t->tid, code, arg, t->saved_sp);

    Q_append(&runq, t);
    return t;
}


// exit current thread.
//   - if no more threads, switch to the scheduler.
//   - otherwise context switch to the new thread.
//     make sure to set cur_thread correctly!
void rpi_exit(int exitcode) {
    RZ_CHECK();

    // if you switch back to the scheduler thread:
    if (Q_empty(&runq)) {
        th_trace("done running threads, back to scheduler\n");
        rpi_cswitch(&cur_thread->saved_sp, scheduler_thread->saved_sp);
    } else {
        rpi_thread_t* old_thread = cur_thread;
        th_free(old_thread);
        cur_thread = Q_pop(&runq);
        rpi_cswitch(&old_thread->saved_sp, cur_thread->saved_sp);
    }

    
    not_reached();
    
}

// yield the current thread.
//   - if the runq is empty, return.
//   - otherwise: 
//      * add the current thread to the back 
//        of the runq (Q_append)
//      * context switch to the new thread.
//        make sure to set cur_thread correctly!
void rpi_yield(void) {
    RZ_CHECK();
    // NOTE: if you switch to another thread: print the statement:
    //     th_trace("switching from tid=%d to tid=%d\n", old->tid, t->tid);
    if (Q_empty(&runq)) {
        return;
    } else {
        // th_trace("address of cur thread is %p\n", cur_thread);
        Q_append(&runq, cur_thread);
        rpi_thread_t* old_thread = cur_thread;
        cur_thread = Q_pop(&runq);
        // th_trace("the queue has %d threads\n", Q_nelem(&runq));
        // th_trace("address of cur thread is %p\n", cur_thread);
        // th_trace("address of old thread is %p\n", old_thread);
        th_trace("switching from tid=%d to tid=%d\n", old_thread->tid, cur_thread->tid);
        rpi_cswitch(&old_thread->saved_sp, cur_thread->saved_sp);
    }
}

/*
 * starts the thread system.  
 * note: our caller is not a thread!  so you have to 
 * create a fake thread (assign it to scheduler_thread)
 * so that context switching works correctly.   your code
 * should work even if the runq is empty.
 */
void rpi_thread_start(void) {
    RZ_CHECK();
    th_trace("starting threads!\n");

    // no other runnable thread: return.
    //th_trace("Is the queue is empty: %d\n", Q_empty(&runq));
    if(Q_empty(&runq))
        goto end;

    // setup scheduler thread block.
    if(!scheduler_thread) {
        scheduler_thread = th_alloc();
        // HOW DOES IT KNOW TO GO TO END??
        // scheduler_thread->saved_sp = scheduler_thread->stack + THREAD_MAXSTACK - 9;
        // scheduler_thread->saved_sp[LR_OFFSET] = (uintptr_t) &&end;
        // cur_thread = Q_pop(&runq);
        // unsigned ans;
        // asm volatile ("mov lr, %0" :: "r" (&&end));
        // asm volatile ("mov %0, lr" : "=r" (ans));
        // rpi_cswitch(&scheduler_thread->saved_sp, cur_thread->saved_sp);
    }
    cur_thread = Q_pop(&runq);
    rpi_cswitch(&scheduler_thread->saved_sp, cur_thread->saved_sp);


end:
    RZ_CHECK();
    // th_trace("thread id is %d\n", tid);
    //scheduler_thread = NULL;
    // if not more threads should print:
    th_trace("done with all threads, returning\n");
}

// helper routine: can call from assembly with r0=sp and it
// will print the stack out.  it then exits.
// call this if you can't figure out what is going on in your
// assembly.
void rpi_print_regs(uint32_t *sp) {
    // use this to check that your offsets are correct.
    printk("cur-thread=%d\n", cur_thread->tid);
    printk("sp=%p\n", sp);

    // stack pointer better be between these.
    printk("stack=%p\n", &cur_thread->stack[THREAD_MAXSTACK]);
    assert(sp < &cur_thread->stack[THREAD_MAXSTACK]);
    assert(sp >= &cur_thread->stack[0]);
    for(unsigned i = 0; i < 9; i++) {
        unsigned r = i == 8 ? 14 : i + 4;
        printk("sp[%d]=r%d=%x\n", i, r, sp[i]);
    }
    clean_reboot();
}
