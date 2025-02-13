// test code for checking the interrupts. 
// see <test-interrupts.h> for additional routines.
#include "test-interrupts.h"
#include "timer-interrupt.h"
#include "vector-base.h"

volatile int n_interrupt;

static interrupt_fn_t interrupt_fn;

// default vector: just forwards it to the registered
// handler see <test-interrupts.h> and the given test.
void interrupt_vector(unsigned pc) { // this is the function handler in the interrupt table
    // trace("in interrupt vector \n");
    dev_barrier();
    n_interrupt++;

    if(!interrupt_fn(pc))
        panic("should have no other interrupts?\n");

    dev_barrier();
}

// initialize all the interrupt stuff.  client passes in the 
// gpio int routine <fn>
//
// make sure you understand how this works.
void test_startup(init_fn_t init_fn, interrupt_fn_t int_fn) {
    output("\tImportant: must loop back (attach a jumper to) pins 20 & 21\n");
    output("\tImportant: must loop back (attach a jumper to) pins 20 & 21\n");
    output("\tImportant: must loop back (attach a jumper to) pins 20 & 21\n");
    // initialize.
    extern uint32_t interrupt_vec[];
    int_vec_init(interrupt_vec);

    gpio_set_output(out_pin);
    gpio_set_input(in_pin);

    init_fn();
    interrupt_fn = int_fn;
    // in case there was an event queued up clear it.
    gpio_event_clear(in_pin);

    // start global interrupts.
    cpsr_int_enable();

}


/********************************************************************
 * falling edge.
 */

volatile int n_falling;

// check if there is an event, check if it was a falling edge.
int falling_handler(uint32_t pc) {
    int fallingEdge = 0;
    if (gpio_read(in_pin) == DEV_VAL32(0)) {
        fallingEdge = 1;
    }

    if (gpio_event_detected(in_pin) && fallingEdge) {
        n_falling++;
        gpio_event_clear(in_pin); // why out pin
        return 1;
    }
    return 0;
}

void falling_init(void) {
    gpio_write(out_pin, 1);
    gpio_int_falling_edge(in_pin); // start looking for falling edge
}

/********************************************************************
 * rising edge.
 */

volatile int n_rising;

// check if there is an event, check if it was a rising edge.
int rising_handler(uint32_t pc) {
    int risingEdge = 0;
    if (gpio_read(in_pin) == DEV_VAL32(1)) {
        risingEdge = 1;
    }
    if (gpio_event_detected(in_pin) && risingEdge) {
        n_rising++;
        gpio_event_clear(in_pin); // why out pin
        return 1;
    } 
    return 0;
} // TODO: how does it know to go to rising handler vs falling handler

void rising_init(void) {
    gpio_write(out_pin, 0);
    gpio_int_rising_edge(in_pin);
}

/********************************************************************
 * timer interrupt
 */

void timer_test_init(void) {
    // turn on timer interrupts.
    rising_init();
    falling_init();
    timer_init(1, 0x4);
}

static volatile unsigned cnt, period, period_sum;

// make sure this gets called on each timer interrupt.
// return 1 if only interrupt
int timer_test_handler(uint32_t pc) {
    n_interrupt += 1;
    // trace("in timer test handler \n");
    dev_barrier();
    // should look very similar to the timer interrupt handler.
    unsigned pending = GET32(IRQ_basic_pending); // this is for timer/hardware devices pending table
    if ((pending & ARM_Timer_IRQ) == 0)
        return 0;

    PUT32(ARM_Timer_IRQ_Clear, 1);

    // dev_barrier();
    // static unsigned last_clk = 0;
    // unsigned clk = timer_get_usec_raw();
    // period = last_clk ? clk - last_clk : 0;
    // period_sum += period;
    // last_clk = clk;

    dev_barrier();
    // trace("at end of handler \n");
    return 1;
}


/*
step 1: create interrupt table that jumps to interrupt vector
interrupt vector jumps to handler function at pc 
(timer test handler and falling/rising edge handler)

step 2: timer test handler 
- turn timer interrupts on: interrupt at every clock cycle

step 3: falling/rising edge handler
- check what current pin is and check if an event has happened

*/