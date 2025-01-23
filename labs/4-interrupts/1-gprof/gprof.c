/*
 * use interrupts to implement a simple statistical profiler.
 *	- interrupt code is a replication of ../timer-int/timer.c
 *	- you'll need to implement kmalloc so you can allocate
 *	  a histogram table from the heap.
 *	- implement functions so that given a pc value, you can increment
 *	  its associated count
 */
#include "rpi.h"

// pulled the interrupt code into these header files.
#include "rpi-interrupts.h"
#include "timer-int.h"

// defines C externs for the labels defined by the linker script
// libpi/memmap.
//
// you can use these to get the size of the code segment,
// data segment, etc.
#include "memmap.h"

/*************************************************************
 * gprof implementation:
 *	- allocate a table with one entry for each instruction.
 *	- gprof_init(void) - call before starting.
 *	- gprof_inc(pc) will increment pc's associated entry.
 *	- gprof_dump will print out all samples.
 */

static unsigned hist_n, pc_min, pc_max;
static volatile unsigned *hist;

// - compute <pc_min>, <pc_max> using the
//   <libpi/memmap> symbols:
//   - use for bounds checking.
// - allocate <hist> with <kmalloc> using <pc_min> and
//   <pc_max> to compute code size.
static unsigned gprof_init(void) {
  pc_min = (uintptr_t)__code_start__;     // 0x8000
  pc_max = (uintptr_t)__code_end__;       // 0x924c or around there
  hist_n = __code_end__ - __code_start__; // 11,000 ish big
  hist_n++; // we want to include self for number of bytes
  printk("pc min %d, pc max %d \n", pc_min, pc_max);
  printk("number of bytes allocated %d \n", hist_n);
  // align to 4
  hist = (unsigned *)kmalloc_aligned(hist_n, 4);
  //   printk("heap address is %p\n", hist);
  //   for (int i = 0; i < hist_n; i++) {
  //     printk("hist[%d] = %d\n", i, hist[i]);
  //   }
  return 0;
}

// increment histogram associated w/ pc.
//    few lines of code
static void gprof_inc(unsigned pc) {
  assert(pc >= pc_min && pc <= pc_max);
  *(hist + ((pc - pc_min) / 4)) += 1;
}

// print out all samples whose count > min_val
//
// make sure gprof does not sample this code!
// we don't care where it spends time.
//
// how to validate:
//  - take the addresses and look in <gprof.list>
//  - we expect pc's to be in GET32, PUT32, different
//    uart routines, or rpi_wait.  (why?)
static void gprof_dump(unsigned min_val) {
  for (unsigned i = 0; i < hist_n; i++) {
    if (hist[i] > min_val) {
      volatile unsigned pc = pc_min + (i * 4);
      if (pc >= 0x8038 && pc <= 0x809c) {
        continue;
      }
      printk("address %p with value %d is being called a lot! \n", pc, hist[i]);
    }
  }
}

/**************************************************************
 * timer interrupt code from before, now calls gprof update.
 */
// Q: if you make not volatile?
static volatile unsigned cnt;
static volatile unsigned period;

// client has to define this.
void interrupt_vector(unsigned pc) {
  dev_barrier();
  unsigned pending = GET32(IRQ_basic_pending);
  if ((pending & ARM_Timer_IRQ) == 0)
    return;

  PUT32(ARM_Timer_IRQ_Clear, 1);
  cnt++;

  // increment the counter for <pc>.
  gprof_inc(pc);

  // this doesn't need to stay here.
  static unsigned last_clk = 0;
  unsigned clk = timer_get_usec();
  period = last_clk ? clk - last_clk : 0;
  last_clk = clk;

  dev_barrier();
}

// trivial program to test gprof implementation.
// 	- look at output: do you see weird patterns?
void notmain() {
  interrupt_init();
  timer_init(16, 0x100);

  // Q: if you move these below interrupt enable?
  uint32_t oneMB = 1024 * 1024;
  kmalloc_init_set_start((void *)oneMB, oneMB);
  gprof_init();

  printk("gonna enable ints globally!\n");
  enable_interrupts();

  // caches_enable(); 	// Q: what happens if you enable cache?
  unsigned iter = 0;
  while (cnt < 200) {
    printk("iter=%d: cnt = %d, period = %dusec, %x\n", iter, cnt, period,
           period);
    iter++;
    if (iter % 10 == 0)
      gprof_dump(2);
  }
}
